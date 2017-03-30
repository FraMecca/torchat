#pragma once
#include "include/fd.h"
#include "include/libdill.h"
#include <stdio.h>
#include "include/libdillimpl.h"
void *torchatproto_hquery(struct hvfs *hvfs,const void *id);
void *torchatproto_hquery(struct hvfs *hvfs,const void *type);
void torchatproto_hclose(struct hvfs *hvfs);
void torchatproto_hclose(struct hvfs *hvfs);
int torchatproto_hdone(struct hvfs *hvfs,int64_t deadline);
int torchatproto_hdone(struct hvfs *hvfs,int64_t deadline);
int torchatproto_attach(int s);
int torchatproto_detach(int h);
int socket_create(int port,int64_t deadline);
ssize_t torchatproto_mrecv(int h,void *buf,size_t maxLen,int64_t deadline);
ssize_t torchatproto_msend(int h,void *buf,size_t len,int64_t deadline);
