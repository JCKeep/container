#ifndef CONTAINER_CONFIG_H
#define CONTAINER_CONFIG_H

#include <container.h>
#include <c_cgroup.h>
#include <cJSON.h>

#define DEFAULT_CONFIG_FILE "./container.json"

struct container_config {
	struct cgroup_context *ctx;
	char *buf;
	char *config_file;
	cJSON *json;
};

extern struct container_config *global_config;

struct config_parse_stat {
	struct container_config *conf;
	unsigned long module;
	char *name;
	cJSON *json;
};

struct container_config *config_create();
int config_init(struct container_config *conf);
int config_parse(struct container_config *conf);

#endif