void
interaction_reply(struct discord_interaction_callback_data response,
		struct discord *client, const struct discord_interaction *event)
{
	CCORDcode ret =
		discord_create_interaction_response(
			client, event->id, event->token,
			&(struct discord_interaction_response){
				.type = DISCORD_INTERACTION_CHANNEL_MESSAGE_WITH_SOURCE,
				.data = &response
			},
			NULL);

	if(ret != CCORD_OK)
		fprintf(stderr, "Responding to %s: %s\n",
				event->data->name, discord_strerror(ret, client));
}

uint32_t
random_color(void)
{
	return((0xFF << (3 * 8))
			| (((rand() % 0x7F) + 0x7F) << (2 * 8))
			| (((rand() % 0x7F) + 0x7F) << 8)
			| ((rand() % 0x7F) + 0x7F));
}

float
random_float(void)
{
	return(rand() / (float) RAND_MAX);
}

struct string_view
get_json_token(char *buffer, jsmntok_t token)
{
	return((struct string_view){
		.ptr = buffer + token.start,
		.len = token.end - token.start
	});
}

void
url_remove_backslashes(char *string)
{
	int len = strlen(string);

	for(int i = 0; i < len; ++i){
		if(string[i] == '\\'){
			memmove(string + i, string + i + 1, len - i);
			--i;
			--len;
		}
	};
}

void
parse_newlines(char *string)
{
	int len = strlen(string);

	for(int i = 0; i < len; ++i){
		if(string[i] == '\\' && string[i + 1] == 'n'){
			string[i] = '\n';
			memmove(string + i + 1, string + i + 2, len - i - 1);
			--len;
		}
	}
}
