#define OLIVEC_IMPLEMENTATION
#define MSF_GIF_IMPL

#define MSF_GIF_MALLOC(arena, len) ma_allocate_impl(arena, len, alignof(max_align_t))
/* Assuming realloc only right after malloc */
#define MSF_GIF_REALLOC(arena, mem, old_len, new_len) \
	(((struct memory_arena *) arena)->end += (new_len) - (old_len), mem)
#define MSF_GIF_FREE(...) do{}while(0)

#define arrlen(arr) (sizeof(arr) / sizeof(arr[0]))

#define print_logf(fmt, ...)						\
	do{								\
		struct tm *datetime =					\
			localtime(&(time_t){ time(NULL) });		\
		fprintf(stderr, "[%d-%02d-%02d %02d:%02d:%02d] "	\
			fmt "\n",					\
			datetime->tm_year + 1900, datetime->tm_mon + 1,	\
			datetime->tm_mday, datetime->tm_hour,		\
			datetime->tm_min, datetime->tm_sec,		\
			__VA_ARGS__);					\
	}while(0)

#define print_log(str) print_logf(str "%s", "")

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
		fprintf(stderr, "Registering commands: %s\n",
				discord_strerror(ret, client));

	print_logf("Logged in as %s!", event->user->username);
	print_logf("%ld commands registered", arrlen(commands));
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
		fprintf(stderr, "Initializing curl: %s\n",
				curl_easy_strerror(ret));
		return(1);
	}

	print_log("Starting bot");
	discord_run(client);

	return(0);
}
