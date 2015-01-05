#include <fcgiapp.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

#include <pthread.h>

// For JSONs
#include "frozen.h"

// For redis
#include "credis.h"

#define ERR_NO_SOCKET 85

#define NUM_THREADS 4
#define BUF_CAP 1024

#define consolelog(x) fprintf(fp, x)

const char* sockpath = "/tmp/fcgicpp.sock";

void do_404(FCGX_Request* request) {
  FCGX_PutS("Status: 404\r\n", request->out);
  FCGX_PutS("\r\n", request->out);
  FCGX_PutS("{ \"status\": \"some 404 error happened\" }", request->out);
  FCGX_Finish_r(request); 
}

void do_500(FCGX_Request* request) {
  FCGX_PutS("Status: 500\r\n", request->out);
  FCGX_PutS("\r\n", request->out);
  FCGX_PutS("{ \"status\": \"some error happened\" }", request->out);
  FCGX_Finish_r(request); 
}

void do_200(FCGX_Request* request) {
  FCGX_PutS("Status: 200\r\n", request->out);
  FCGX_PutS("\r\n", request->out);
  FCGX_PutS("{ \"status\": 200 }", request->out);
  FCGX_Finish_r(request); 
}

void do_put(char* user, char* lat, char* lon, FCGX_Request* request, REDIS* redis, FILE* fp) {
  char buf[BUF_CAP];

  if (!*redis) {
    do_500(request);
    return;
  }

  sprintf(buf, "{ \"lat\": \"%s\", \"lon\": \"%s\" }", lat, lon);

  fprintf(fp, "buf to PUT: %s %zu|| %s\n", buf, strlen(buf), user);
  fflush(fp);

  credis_set(*redis, user, buf);
  do_200(request);
}

void do_del(char* user, FCGX_Request* request, REDIS* redis, FILE* fp) {
  if (!*redis) {
    do_500(request);
    return;
  }

  fprintf(fp, "deleting user: %s\n", user);
  fflush(fp);
  credis_del(*redis, user);
  do_200(request);
}

void do_get(char* user, FCGX_Request* request, REDIS* redis, FILE* fp) {
  char* coords;

  if (!*redis) {
    fprintf(fp, "no redis!!!\n");
    fflush(fp);
    do_500(request);
    return;
  }

  credis_get(*redis, user, &coords);

  if (!coords) {
    fprintf(fp, "no coords for user: %s\n", user);
    fflush(fp);
    do_404(request);
    return;
  }

  FCGX_PutS("Status: 200\r\n", request->out);
  FCGX_PutS("\r\n", request->out);
  FCGX_PutS(coords, request->out);
  FCGX_Finish_r(request); 
}

char* extract_user_from_uri(char* uri, FILE* fp) {
  char* uri_c;
  char* token;

  uri_c = strdup(uri);

  token = strtok(uri_c, "/");

  fprintf(fp, "token1: %s\n", token);
  fflush(fp);

  if (strcmp (token, "loc") != 0) {
    return 0;
  }

  token = strtok(NULL, "/");

  fprintf(fp, "token2: %s\n", token);
  fflush(fp);

  if (strcmp (token, "get") != 0) {
    return 0;
  }

  token = strtok(NULL, "/");

  fprintf(fp, "token3: %s\n", token);
  fflush(fp);
  if (strcmp (token, "") == 0) {
    return NULL;
  }

  return token;
}

int is_putdel(char* method, char* uri, char* lasttoken, FILE* fp) {
  char* uri_c;
  char* token;

  if (strcmp (method, "POST") != 0) {
    return 0;
  }

  uri_c = strdup(uri);

  token = strtok(uri_c, "/");

  if (strcmp (token, "loc") != 0) {
    return 0;
  }

  token = strtok(NULL, "/");

  if (strcmp (token, lasttoken) != 0) {
    return 0;
  }

  fprintf(fp, "it is PUT!\n");
  fflush(fp);

  return 1;
}

int is_get(char* method, char* uri, FILE* fp) {
  if (strcmp (method, "GET") != 0) {
    fprintf(fp, "wtf1");

    fflush(fp);
    return 0;
  }


  fprintf(fp, "here\n");
  fflush(fp);
  if (!extract_user_from_uri(uri, fp)) {
    return 0;
  }

  return 1;
}

int is_put(char* method, char* uri, FILE* fp) {
  return is_putdel(method, uri, "put", fp);
}

int is_del(char* method, char* uri, FILE* fp) {
  return is_putdel(method, uri, "del", fp);
}

struct json_token* extract_body(FCGX_Stream* in, FILE* fp) {
  char buf[BUF_CAP];
  struct json_token* parsed_json;

  FCGX_GetLine(buf, BUF_CAP, in);

  fprintf(fp, "about to extract body:: %s\n", buf);
  fflush(fp);

  if (*buf) {
    
    parsed_json = parse_json2(buf, strlen(buf));

    fprintf(fp, "extracted something??\n");
    fflush(fp);
    return parsed_json;
  } else {
    return NULL;
  }
}

void safe_extract(struct json_token* body, char *buf, char* key) {
  struct json_token* token;

  token = find_json_token(body, key);

  sprintf(buf, "%.*s", token->len, token->ptr);
}

void extract_user_from_body(struct json_token* body, char *buf) {
  return safe_extract(body, buf, "user");
}

void extract_lon_from_body(struct json_token* body, char *buf) {
  return safe_extract(body, buf, "lon");
}

void extract_lat_from_body(struct json_token* body, char *buf) {
  return safe_extract(body, buf, "lat");
}

struct thread_info {
  int sfd;
  int i;
};

void* start_thread(void *arg) {
  struct thread_info* info = (struct thread_info *)arg;
  int socket_fd = info->sfd;
  int log_i = info->i;
  struct json_token* body;
  char* method;
  char* uri;
  char* user_get;
  char user[BUF_CAP];
  char lat[BUF_CAP];
  char lon[BUF_CAP];

  int rc;

  char logname[BUF_CAP];
  sprintf(logname, "/tmp/log.%d.log", log_i);

  FILE* log = fopen(logname, "w");

  fprintf(log, "yo%d\n", log_i);

  REDIS redis;

  FCGX_Request request;

  FCGX_InitRequest(&request, socket_fd, 0);

  redis = credis_connect(NULL, 6379, 2000);
  credis_auth(redis, "mobileshakeredis12345");

  fprintf(log, "i seem to have connected to redis\n");
  fflush(log);
  while (1) {

    static pthread_mutex_t accept_mutex = PTHREAD_MUTEX_INITIALIZER; 
    pthread_mutex_lock(&accept_mutex);
    rc = FCGX_Accept_r(&request);
    pthread_mutex_unlock(&accept_mutex);
    
    if (rc < 0) {
      break;
    }

    method = FCGX_GetParam("REQUEST_METHOD", request.envp);
    uri = FCGX_GetParam("REQUEST_URI", request.envp);

    fprintf(log, "got request of uri and method: %s and %s\n", uri, method);
    fflush(log);

    if (is_get(method, uri, log)) {
      fprintf(log, "get succeeded");
      user_get = extract_user_from_uri(uri, log);
      fprintf(log, "user: %s\n", user_get);
      fflush(log);
      do_get(user, &request, &redis, log);
    } else if (is_put(method, uri, log)) {
      body = extract_body(request.in, log);

      fprintf(log, "extracted body   %s\n", "aaa");
      extract_user_from_body(body, user);
      fprintf(log, "user for put is: %s", user);
      extract_lat_from_body(body, lat);
      fprintf(log, "lat for put is: %s", lat);
      extract_lon_from_body(body, lon);
      fprintf(log, "lon for put is: %s", lon);
      fflush(log);

      do_put(user, lat, lon, &request, &redis, log);
    } else if (is_del(method, uri, log)) {
      body = extract_body(request.in, log);
      extract_user_from_body(body, user);
      do_del(user, &request, &redis, log);
    } else {
      do_404(&request);
    }
  }

  return NULL;
}

int main(void) {
  int socket_fd;
  unsigned int i;
  struct thread_info* info;

  FCGX_Init();
  chmod(sockpath, 0777);
  socket_fd = FCGX_OpenSocket(sockpath, 128);

  if (socket_fd < 0) {
    exit(ERR_NO_SOCKET);
  }

  pthread_t threads[NUM_THREADS];
  for (i = 0; i < NUM_THREADS; i++) {
    info = malloc(sizeof(struct thread_info));
    info->sfd = socket_fd;
    info->i = i;
    pthread_create(&threads[i], NULL, start_thread, info);
  }

  for (i = 0; i < NUM_THREADS; i++) {
    pthread_join(threads[i], NULL);
  }

  return 0;
}