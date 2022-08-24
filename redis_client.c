#include <errno.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <hiredis/hiredis.h>
#include "config.h"
#include "redis_client.h"



#define BUFFER_SIZE (256*1024) // 256kB


int redis_init(void) {
	return 0;
}

int redis_close(void) {
	return 0;
}

/*
 * Given a service name, queries Consul API and returns numeric address
 */

int redis_lookup(const char *service, const char *tenant, const char *name, char *data) {

    redisContext *c;
    redisReply *reply;

    struct timeval timeout = { 1, 500000 }; // 1.5 seconds

           
#if DEBUG
    printf( "@ %s::(in) name=%s\n", __FUNCTION__, name ) ;
#endif

// #if defined(REDIS_SOCKET)
//    c = redisConnectUnixWithTimeout(REDIS_SOCKET, timeout);
// #else
    c = redisConnectWithTimeout(REDIS_HOST, REDIS_PORT, timeout);
// #endif

    if (c == NULL || c->err) {
        if (c) {
            printf("Connection error: %s\n", c->errstr);
            redisFree(c);
        } else {
            printf("Connection error: can't allocate redis context\n");
        }
	goto fail;
        // exit(1);
    }


    // AUTH
    reply = redisCommand(c,"AUTH %s", REDIS_PASSWORD);
    if (reply == NULL) {
        // wrong redis password
	printf("wrong redis password");
        //// ap_log_error(APLOG_MARK, APLOG_ERR, 0, s,"wrong redis password");
        //// free_redis_ctx(ctx, s);
        redisFree(c);
        goto fail;
        //// return NULL;
    }
    // DEBUG
    //ap_log_error(APLOG_MARK, APLOG_ERR, 0, s,"redis reply: AUTH, type[%d], str[%s]", reply->type, reply->str);
    freeReplyObject(reply);

#if DEBUG
#if USE_TENANT
    printf( "@ %s::(in) cmd=GET %s/%s/%s\n", __FUNCTION__, service, tenant, name );
	if (tenant == NULL) {
		redisFree(c);
        goto fail;
	}
#else
    printf( "@ %s::(in) cmd=GET %s/%s\n", __FUNCTION__, service, name );
#endif
#endif 

#if USE_TENANT
	if (tenant == NULL) {
		redisFree(c);
        goto fail;
	}
    reply = redisCommand(c,"GET %s/%s/%s", service, tenant, name);
#else
    reply = redisCommand(c,"GET %s/%s", service, name);
#endif    
    if (c == NULL || c->err) {
	#if DEBUG
        if (c) {
	    char *ret;	
	    ret = strstr(c->errstr, "erver closed the connectio");
	    if (ret) {
	      printf("Server closed the connection, have you exceeded a quota?\n");
            } else {
              printf("Connection error : %s\n", c->errstr);
	    }
            redisFree(c);
        } else {
            printf("Connection error: can't allocate redis context\n");
        }
	#endif
        goto fail;
        // exit(1);
    }
#if DEBUG
    printf("@ %s::(out) value=%s\n", __FUNCTION__, reply->str);
#endif
	if (reply->type == REDIS_REPLY_STRING) {
    	strncpy(data,reply->str,reply->len+1);  
 	} else {
        freeReplyObject(reply);
		redisFree(c);
		goto fail;
	}



    freeReplyObject(reply);
    redisFree(c);
    return 0;


fail:
#if DEBUG
     printf( "@ %s::(out) Error\n", __FUNCTION__ ); 
#endif
    return 1;
}




