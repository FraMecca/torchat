#include <string.h>
#include "include/json/cJSON.h"
#include "lib/tc_json.h"

size_t
json_length (JSON *json)
{
	JSON *j = json->child;
	size_t len = 0;
	while (j) {
		len++;
		j = j->next;
	}
	return len;
}

void *
json_get (cJSON *json, char *k)
{
	/*cJSON *j = cJSON_CreateObject ();*/
	/*cJSON_AddStringToObject (j, "msg", "msgV");*/
	/*cJSON_AddNumberToObject (j, "num", 1);*/
	JSON *j = json->child;
	// no json pointers
	while (j && (strcmp (j->string, k) != 0)) j = j->next;
	if (!j) return NULL;

	switch (j->type) {
		// no nested json
		case cJSON_String:
			return j->valuestring;
        case cJSON_False:
        case cJSON_True:
        case cJSON_Number:
        	return (void *) &j->valueint;
        /*case cJSON_NULL:*/
        /*case cJSON_Raw:*/
        /*case cJSON_Array:*/
        /*case cJSON_Object:*/
        default:
        	return NULL;
    }
}
