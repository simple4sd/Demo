#include <stdio.h>
#include <unistd.h>
#include <cJSON.h>

int parse_policy(char *policy)
{
	if (policy == NULL)
		return -1;

	const cJSON *keyfile   = NULL;
	const cJSON *file_path = NULL;
	cJSON *policy_json = cJSON_Parse(policy);
	if (policy_json == NULL) {
		const char *error_ptr = cJSON_GetErrorPtr();
		if (error_ptr != NULL)
			printf("parse error %s\n", error_ptr);
		goto out;
	}

	keyfile = cJSON_GetObjectItemCaseSensitive(policy_json, "keyfile");
	cJSON_ArrayForEach(file_path, keyfile) {
		if (cJSON_IsString(file_path)) {
			printf("file path is %s\n", file_path->valuestring);
		}
	}
out:
	cJSON_Delete(policy_json);
	return 0;
}

int main()
{
	char policy[] = "{\"keyfile\":[\"/etc/init.d\", \"/tmp/abc\"]}";
	printf("policy = %s\n", policy);
	parse_policy(policy);
	return 0;
}