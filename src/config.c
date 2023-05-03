#include <stdio.h>
#include <string.h>
#include <config.h>

void cJSON_test()
{
    FILE *fp = fopen("container.json", "r");
    if (!fp) {
        return;
    }

    char buf[4096];
    int len = 0;
    char c;

    memset(buf, 0, sizeof(buf));

    while ((c = fgetc(fp)) != EOF) {
        buf[len++] = c;
    }

    cJSON *json = cJSON_Parse(buf);
    if (json == NULL) {
        printf("Failed to parse JSON string.\n");
        return;
    }

    cJSON *name1 = cJSON_GetObjectItem(json, "test");
    if (name1 != NULL) {
        printf("Name: %s\n", name1->valuestring);
    }

    cJSON_Delete(json);
    fclose(fp);
    return;
}