#define OLIVEC_IMPLEMENTATION
#define MSF_GIF_IMPL

#define MSF_GIF_MALLOC(arena, len) ma_allocate_impl(arena, len, alignof(max_align_t))
#define MSF_GIF_REALLOC(arena, mem, old_len, new_len) (((struct memory_arena *) arena)->end += (new_len) - (old_len), mem)
#define MSF_GIF_FREE(...) do{}while(0)

#define arrlen(arr) (sizeof(arr) / sizeof(arr[0]))

#include <stdlib.h>
#include <time.h>
#include <math.h>
#include <concord/discord.h>
#include <concord/jsmn.h>
#include <cglm/struct.h>
#include <curl/curl.h>
#include <cut/cut.c>
#include <olive.c/olive.c>
#include <msf_gif/msf_gif.h>

#include "util.c"
#include "pong.c"
#include "wheel.c"
#include "mal.c"

struct discord_application_command commands[] = {
	{
		.name = "ping",
		.description = "Responds with \"pong\"",
	},
	{
		.name = "wheel",
		.description = "Spin the wheel",
		.options = &(struct discord_application_command_options){
			.array = (struct discord_application_command_option[]){
				{
					.type = DISCORD_APPLICATION_OPTION_STRING,
					.name = "choices",
					.description = "Comma-separated list of choices",
					.required = true,
				}
			},
			.size = 1
		}
	},
	{
		.name = "character",
		.description = "Get random character from MAL"
	}
};

void (*callbacks[])(struct memory_arena *, struct discord *, const struct discord_interaction *) = {
	pong,
	wheel,
	mal_character
};

void
on_ready(struct discord *client, const struct discord_ready *event)
{
	CCORDcode ret =
		discord_bulk_overwrite_global_application_commands(client,
				event->application->id,
				&(struct discord_application_commands){
					.array = commands,
					.size = arrlen(commands)
				},
				NULL);

	if(ret != CCORD_OK && ret != CCORD_PENDING)
		printf("Registering commands: %s\n",
			   discord_strerror(ret, client));

	printf("Logged in as %s!\n", event->user->username);
	printf("%ld commands registered\n", arrlen(commands));
}

void
on_interaction(struct discord *client, const struct discord_interaction *event)
{
	if(event->type != DISCORD_INTERACTION_APPLICATION_COMMAND)
		return;

	for(int i = 0; i < (int) arrlen(commands); ++i){
		if(strcmp(event->data->name, commands[i].name) == 0){
			struct memory_arena arena = {};
			callbacks[i](&arena, client, event);
			ma_free(arena);
			break;
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
		printf("Initializing curl: %s\n",
			   curl_easy_strerror(ret));
		return(1);
	}

	printf("Starting bot\n");

	CCORDcode ret2 = discord_run(client);

	if(ret2 != CCORD_OK)
		printf("Running client: %s\n",
			   discord_strerror(ret2, client));

	printf("Exiting...\n");

	return(0);
}
