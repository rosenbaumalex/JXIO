/*
** Copyright (C) 2013 Mellanox Technologies
**
** Licensed under the Apache License, Version 2.0 (the "License");
** you may not use this file except in compliance with the License.
** You may obtain a copy of the License at:
**
** http://www.apache.org/licenses/LICENSE-2.0
**
** Unless required by applicable law or agreed to in writing, software
** distributed under the License is distributed on an "AS IS" BASIS,
** WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND,
** either express or implied. See the License for the specific language
** governing permissions and  limitations under the License.
**
*/

#include <string.h>
#include <map>

#include "Events.h"


typedef enum {
	EVENT_SESSION_ERROR = 0,
	EVENT_MSG_ERROR_SERVER = 1,
	EVENT_MSG_ERROR_CLIENT = 2,
	EVENT_SESSION_ESTABLISHED = 3,
	EVENT_REQUEST_RECEIVED = 4,
	EVENT_REPLY_RECEIVED = 5,
	EVENT_SESSION_NEW = 6,
	EVENT_MSG_SEND_COMPLETE = 7,
	EVENT_FD_READY = 8,
	EVENT_LAST
} event_type_t;


Events::Events()
{
	this->size = 0;
}

int Events::writeOnSessionErrorEvent(char *buf, void *ptrForJava, struct xio_session_event_data *event_data)
{
	struct event_struct* event = (struct event_struct*)buf;
	event->type = htonl(EVENT_SESSION_ERROR);
	event->ptr = htobe64(intptr_t(ptrForJava));
	event->event_specific.session_error.error_type = htonl(event_data->event);
	int reason = 0;
	if (event_data->reason){
		reason = event_data->reason - XIO_BASE_STATUS + 1;
	}
	event->event_specific.session_error.error_reason = htonl (reason);

	this->size = sizeof(struct event_session_error) + sizeof((event_struct *)0)->type + sizeof((event_struct *)0)->ptr;

	return this->size;
}

int Events::writeOnSessionEstablishedEvent (char *buf, void *ptrForJava, struct xio_session *session,
			struct xio_new_session_rsp *rsp)
{
	struct event_struct* event = (struct event_struct*)buf;
	event->type = htonl(EVENT_SESSION_ESTABLISHED);
	event->ptr = htobe64(intptr_t(ptrForJava));
	this->size = sizeof((event_struct *)0)->type + sizeof((event_struct *)0)->ptr;
	return this->size;
}

int Events::writeOnNewSessionEvent(char *buf, void *ptrForJava, struct xio_session *session,
			struct xio_new_session_req *req)
{
	void* p1 =  session;
	struct event_struct* event = (struct event_struct*)buf;

	event->type = htonl(EVENT_SESSION_NEW);
	event->ptr = htobe64(intptr_t(ptrForJava));
	event->event_specific.new_session.ptr_session = htobe64(intptr_t(p1));
	event->event_specific.new_session.uri_len = htonl(req->uri_len);

	//copy data so far
	this->size = sizeof((event_struct *)0)->type + sizeof((event_struct *)0)->ptr +
			sizeof((struct event_new_session *)0)->ptr_session + sizeof((struct event_new_session *)0)->uri_len;

	//copy first string
	strcpy(buf + this->size, req->uri);
	size += req->uri_len;

	//calculate ip address
	int len;
	char * ip;

	struct sockaddr *ipStruct = (struct sockaddr *)&req->src_addr;

	if (ipStruct->sa_family == AF_INET) {
		static char addr[INET_ADDRSTRLEN];
		struct sockaddr_in *v4 = (struct sockaddr_in *) ipStruct;
		ip = (char *) inet_ntop(AF_INET, &(v4->sin_addr), addr, INET_ADDRSTRLEN);
		len = strlen(ip);
	}
	else if (ipStruct->sa_family == AF_INET6) {
		static char addr[INET6_ADDRSTRLEN];
		struct sockaddr_in6 *v6 = (struct sockaddr_in6 *) ipStruct;
		ip = (char *) inet_ntop(AF_INET6, &(v6->sin6_addr), addr, INET6_ADDRSTRLEN);
		len = INET6_ADDRSTRLEN;
	}
	else {
		LOG_ERR("can not get src ip");
		return 0;
	}

	int32_t ip_len = htonl(len);
	memcpy(buf + this->size, &ip_len, sizeof(int32_t));

	this->size += sizeof((struct event_new_session *) 0)->ip_len;
	strcpy(buf + this->size, ip);

	this->size += len;
	return this->size;
}

int Events::writeOnMsgSendCompleteEvent(char *buf, void *ptrForJava, struct xio_session *session,
			struct xio_msg *msg)
{
	struct event_struct* event = (struct event_struct*)buf;

	event->type = htonl(EVENT_MSG_SEND_COMPLETE);
	event->ptr = htobe64(intptr_t(ptrForJava));
	this->size = sizeof((event_struct *)0)->type + sizeof((event_struct *)0)->ptr;
	return this->size;
}

int Events::writeOnMsgErrorEventServer(char *buf, void *ptrForJavaMsg, void* ptrForJavaSession, enum xio_status error)
{
	struct event_struct* event = (struct event_struct*)buf;

    event->type = htonl(EVENT_MSG_ERROR_SERVER);
    event->ptr = htobe64(intptr_t(ptrForJavaMsg));
    int reason = error - XIO_BASE_STATUS + 1;
    event->event_specific.msg_error_server.error_reason = htonl (reason);
    event->event_specific.msg_error_server.ptr_session = htobe64(intptr_t(ptrForJavaSession));
    this->size = sizeof(struct event_msg_error_server) + sizeof((event_struct *)0)->type + sizeof((event_struct *)0)->ptr;
    return this->size;
}


int Events::writeOnMsgErrorEventClient(char *buf, void *ptrForJavaMsg, enum xio_status error)
{
	struct event_struct* event = (struct event_struct*)buf;
	event->type = htonl(EVENT_MSG_ERROR_CLIENT);
	event->ptr = htobe64(intptr_t(ptrForJavaMsg));

	int reason = error - XIO_BASE_STATUS + 1;
	event->event_specific.msg_error_client.error_reason = htonl (reason);

	this->size = sizeof(struct event_msg_error_client) + sizeof((event_struct *)0)->type + sizeof((event_struct *)0)->ptr;
	return this->size;
 }


int Events::writeOnReqReceivedEvent(char *buf, void *ptrForJavaMsg, const int32_t msg_size, void *ptrForJavaSession)
{
	struct event_struct* event = (struct event_struct*)buf;
	event->type = htonl(EVENT_REQUEST_RECEIVED);
	event->ptr = htobe64(intptr_t(ptrForJavaMsg));
	event->event_specific.req_received.msg_size = htonl(msg_size);
	event->event_specific.req_received.ptr_session = htobe64(intptr_t(ptrForJavaSession));
	this->size = sizeof(struct event_req_received) +  sizeof((event_struct *)0)->type + sizeof((event_struct *)0)->ptr;
	return this->size;
}

int Events::writeOnReplyReceivedEvent(char *buf, void *ptrForJavaMsg, const int32_t msg_size)
{
	struct event_struct* event = (struct event_struct*)buf;
	event->type = htonl(EVENT_REPLY_RECEIVED);
	event->ptr = htobe64(intptr_t(ptrForJavaMsg));
	event->event_specific.reply_received.msg_size = htonl(msg_size);
	this->size = sizeof(struct event_reply_received) +  sizeof((event_struct *)0)->type + sizeof((event_struct *)0)->ptr;
	return this->size;
}


int Events::writeOnFdReadyEvent(char *buf, int fd, int epoll_event)
{
	struct event_struct* event = (struct event_struct*)buf;
	event->type = htonl(EVENT_FD_READY);
	event->ptr = 0; //  The java object receiving this event will be the EQH which handles this event_queue
	event->event_specific.fd_ready.fd = htonl(fd);
	event->event_specific.fd_ready.epoll_event = htonl(epoll_event);
	this->size = sizeof(struct event_fd_ready) + sizeof((event_struct *)0)->type + sizeof((event_struct *)0)->ptr;
	return this->size;
}
