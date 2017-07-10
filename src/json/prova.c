#include "../../include/json/cJSON.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>


int
main ()
{
	cJSON *j = cJSON_CreateObject ();
	cJSON_AddStringToObject (j, "msg", "msgV");
	cJSON_AddNumberToObject (j, "num", 1);

	char *r = cJSON_Print (j);
	printf ("%s\n", r);

	char json[] = "{\"msg\":\"prova\",\"int\":1, \"Bool\":true, \"Bool2\":false}";
	cJSON *j2 = cJSON_Parse (json);
	printf ("%s\n", cJSON_Print (j2));
	assert (j2->child->type == cJSON_String);
	assert (j2->child->next->next->next->type == cJSON_False);


	return 0;
}
