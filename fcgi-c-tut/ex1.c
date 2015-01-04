#include "fcgi_stdio.h"
#include <stdlib.h>

void main(void) {
	while(FCGI_Accept() >= 0) {
		printf("Content-Type: text/html\r\n\r\n"
			"hELLO WORLD");
	}
}