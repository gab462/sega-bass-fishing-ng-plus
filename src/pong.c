void
pong(struct memory_arena *,
     struct discord *client, const struct discord_interaction *event)
{
	struct discord_interaction_callback_data response = {
		.content = "pong"
	};

	interaction_reply(response, client, event);
}
