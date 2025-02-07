#include <stdlib.h>
#include <time.h>
#include <math.h>
#include <concord/discord.h>
#include <concord/jsmn.h>
#include <cglm/struct.h>
#include <curl/curl.h>
#include <cut/cut.c>
#define OLIVEC_IMPLEMENTATION
#include <olive.c/olive.c>
#define MSF_GIF_IMPL
#include <msf_gif/msf_gif.h>

#include "util.c"
#include "pong.c"
#include "wheel.c"
#include "mal.c"

typedef void (*cmd_callback)(struct discord *, const struct discord_interaction *);

struct {
	struct {
		struct discord_application_command *ptr;
		int len, cap;
	} cmds;
	struct {
		cmd_callback *ptr;
		int len, cap;
	} callbacks;
} app = {};

void
app_add_cmd(char *name, char *description, cmd_callback callback, struct discord_application_command_options *options)
{
	da_push(&app.cmds, ((struct discord_application_command){
		.name = name,
		.description = description,
		.options = options
	}));

	da_push(&app.callbacks, callback);
}

void
on_ready(struct discord *client, const struct discord_ready *event)
{
	app_add_cmd("ping", "Responds with \"pong\"", pong, NULL);

	struct discord_application_command_options wheel_options = {
		.array = (struct discord_application_command_option[]){
			{
				.type = DISCORD_APPLICATION_OPTION_STRING,
				.name = "choices",
				.description = "Comma-separated list of choices",
				.required = true,
			}
		},
		.size = 1
	};

	app_add_cmd("wheel", "Spin the weel", wheel, &wheel_options);

	app_add_cmd("character", "Random character", character, NULL);

	struct discord_application_commands commands = {
		.array = app.cmds.ptr,
		.size = app.cmds.len
	};

	CCORDcode ret = discord_bulk_overwrite_global_application_commands(client, event->application->id, &commands, NULL);

	if(ret != CCORD_OK)
		fprintf(stderr, "Registering commands: %s\n",
				discord_strerror(ret, client));

	printf("Logged in as %s!\n", event->user->username);
	printf("%d commands registered\n", app.cmds.len);
}

void
on_interaction(struct discord *client, const struct discord_interaction *event)
{
	if(event->type != DISCORD_INTERACTION_APPLICATION_COMMAND)
		return;

	for(int i = 0; i < app.cmds.len; ++i){
		if(strcmp(event->data->name, app.cmds.ptr[i].name) == 0){
			app.callbacks.ptr[i](client, event);
		}
	}
}

int
main(void)
{
	char *token = getenv("TOKEN");
	assert(token != NULL);

	struct discord *client = discord_init(token);

	discord_set_on_ready(client, &on_ready);
	discord_set_on_interaction_create(client, &on_interaction);

	CURLcode ret = curl_global_init(CURL_GLOBAL_ALL);
	if(ret != CURLE_OK){
		fprintf(stderr, "Initializing curl: %s\n",
				curl_easy_strerror(ret));
		return 1;
	}

	printf("Starting bot...\n");
	discord_run(client);

	return(0);
}
