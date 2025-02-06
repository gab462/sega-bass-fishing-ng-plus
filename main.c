#include <concord/discord.h>
#include <cut.c>

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
app_add_cmd(char *name, char *description, cmd_callback callback)
{
	da_push(&app.cmds, ((struct discord_application_command){
		.name = name,
		.description = description
	}));

	da_push(&app.callbacks, callback);
}

void
pong(struct discord *client, const struct discord_interaction *event)
{
	discord_create_interaction_response(
		client, event->id, event->token,
		&(struct discord_interaction_response){
			.type = DISCORD_INTERACTION_CHANNEL_MESSAGE_WITH_SOURCE,
			.data = &(struct discord_interaction_callback_data){
				.content = "pong"
			}
		},
		NULL);
}

void
on_ready(struct discord *client, const struct discord_ready *event)
{
	app_add_cmd("ping", "Responds with \"pong\"", pong);

	struct discord_application_commands commands = {
		.array = app.cmds.ptr,
		.size = app.cmds.len
	};

	discord_bulk_overwrite_global_application_commands(client, event->application->id, &commands, NULL);
}

void
on_interaction(struct discord *client, const struct discord_interaction *event)
{
	if(event->type != DISCORD_INTERACTION_APPLICATION_COMMAND)
		return;

	for (int i = 0; i < app.cmds.len; ++i){
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

	discord_run(client);

	return(0);
}
