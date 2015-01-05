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

const char* sockpath = "127.0.0.1:9000";

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

void do_put(char* user, char* lat, char* lon, FCGX_Request* request, REDIS* redis) {
  char buf[BUF_CAP];

  if (!redis) {
    do_500(request);
    return;
  }

  sprintf(buf, "{ \"lat\": \"%s\", \"lon\": \"%s\" }", lat, lon);

  credis_set(*redis, user, buf);
  do_200(request);
}

void do_del(char* user, FCGX_Request* request, REDIS* redis) {
  if (!redis) {
    do_500(request);
    return;
  }

  credis_del(*redis, user);
  do_200(request);
}

void do_get(char* user, FCGX_Request* request, REDIS* redis) {
  char* coords;

  if (!redis) {
    do_500(request);
    return;
  }

  credis_get(*redis, user, &coords);

  if (!coords) {
    do_404(request);
    return;
  }

  FCGX_PutS("Status: 200\r\n", request->out);
  FCGX_PutS("\r\n", request->out);
  FCGX_PutS(coords, request->out);
  FCGX_Finish_r(request); 
}

char* extract_user_from_uri(char* uri) {
  char* uri_c;
  char* token;

  uri_c = strdup(uri);

  token = strtok(uri_c, "/");

  if (strcmp (token, "loc") != 0) {
    return 0;
  }

  token = strtok(NULL, "/");

  if (strcmp (token, "get") != 0) {
    return 0;
  }

  token = strtok(NULL, "/");

  if (strcmp (token, "") == 0) {
    return NULL;
  }

  return token;
}

int is_putdel(char* method, char* uri, char* lasttoken) {
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

  return 1;
}

int is_get(char* method, char* uri) {
  if (strcmp (method, "GET") != 0) {
    return 0;
  }

  if (!extract_user_from_uri(uri)) {
    return 0;
  }

  return 1;
}

int is_put(char* method, char* uri) {
  return is_putdel(method, uri, "put");
}

int is_del(char* method, char* uri) {
  return is_putdel(method, uri, "del");
}

struct json_token* extract_body(FCGX_Stream* in) {
  char buf[BUF_CAP];
  struct json_token* parsed_json;

  FCGX_GetLine(buf, BUF_CAP, in);

  if (*buf) {
    
    parsed_json = parse_json2(buf, strlen(buf));

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
  struct json_token* body;
  char* method;
  char* uri;
  char* user_get;
  char user[BUF_CAP];
  char lat[BUF_CAP];
  char lon[BUF_CAP];

  int rc;

  REDIS redis;

  FCGX_Request request;

  FCGX_InitRequest(&request, socket_fd, 0);

  redis = credis_connect(NULL, 6379, 2000);
  credis_auth(redis, "mobileshakeredis12345");

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

    if (is_get(method, uri)) {
      user_get = extract_user_from_uri(uri);
      do_get(user_get, &request, &redis);
    } else if (is_put(method, uri)) {
      body = extract_body(request.in);

      extract_user_from_body(body, user);
      extract_lat_from_body(body, lat);
      extract_lon_from_body(body, lon);
      do_put(user, lat, lon, &request, &redis);
    } else if (is_del(method, uri)) {
      body = extract_body(request.in);
      do_del(user, &request, &redis);
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