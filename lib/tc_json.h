#pragma once
#include "include/json/cJSON.h"

#define JSON cJSON
#define json_parse(json) cJSON_Parse(json)
#define json_dump(json) cJSON_Print(json)
#define JSON_new() cJSON_CreateObject ()
#define JSON_add_str(j, k, v) cJSON_AddStringToObject (j, k, v)
#define JSON_add_int(j, k, v) cJSON_AddNumberToObject (j, k, v);

static inline void
destroy_json (cJSON **j)
{
	cJSON_Delete(*j);
}

size_t json_length (JSON *j);
void * json_get (cJSON *json, char *k);
