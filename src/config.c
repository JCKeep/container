#include <container.h>
#include <config.h>

static struct container_config __global_config;
struct container_config *global_config = &__global_config;

struct container_config *config_create()
{
	struct container_config *conf;

	/* we will call several fork(), use shared mem to avoid COW */
	conf =
	    mmap(NULL, sizeof(struct container_config), PROT_READ | PROT_WRITE,
		 MAP_SHARED | MAP_ANONYMOUS, -1, 0);
	if (conf == MAP_FAILED) {
		return NULL;
	}

	memset(conf, 0, sizeof(struct container_config));

	return conf;
}

int config_init(struct container_config *conf)
{
	if (conf->config_file == NULL) {
		conf->config_file = DEFAULT_CONFIG_FILE;
	}

	if (conf->ctx == NULL) {
		conf->ctx = global_cgrp_ctx;
	}

	int rc, fd;
	struct stat st;

	fd = open(conf->config_file, O_RDWR);
	if (fd < 0) {
		BUG();
		return -1;
	}

	rc = fstat(fd, &st);
	if (rc < 0) {
		BUG();
		return -1;
	}

	/* we will call several fork(), use shared mem to avoid COW */
	conf->buf =
	    mmap(NULL, st.st_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	if (conf->buf == MAP_FAILED) {
		BUG();
		goto fail;
	}

	dbg(conf->buf);

	conf->json = cJSON_Parse(conf->buf);

	close(fd);
	return 0;

      fail:
	close(fd);
	return -1;
}

int config_parse(struct container_config *conf)
{
	int ret = 0;
	struct config_parse_stat st;

	cJSON *cgroup_module_config = cJSON_GetObjectItem(conf->json, "cgroup");
	if (cgroup_module_config == NULL) {
		printf("no cgroup config\n");
	} else {
		st.conf = conf;
		st.json = cgroup_module_config;
		st.name = "cgroup";
		st.module = CGRP_MODULE;

		dbg("parsing config");

		ret = st.conf->ctx->parse(st.conf->ctx, &st);
		if (ret < 0) {
			perror("config_parse");
			return -1;
		}
	}

	if (conf->ctx->attach) {
		ret = conf->ctx->attach(conf->ctx);
	}
	if (ret < 0) {
		return -1;
	}

	cJSON_Delete(conf->json);

	return 0;
}

#ifdef CJSON_TEST
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
#endif
