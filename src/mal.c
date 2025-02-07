void
mal_character(struct discord *client, const struct discord_interaction *event)
{
	struct string_buffer sb = http_request("https://api.jikan.moe/v4/random/characters");

	if(sb.ptr == NULL){
		struct discord_interaction_callback_data response = {
			.content = "Request failed"
		};

		interaction_reply(response, client, event);
		return;
	}

	sb_terminate(&sb);

	constexpr int max_tokens = 64;

	jsmn_parser parser;
	jsmntok_t tokens[max_tokens];

	jsmn_init(&parser);

	int json_count = jsmn_parse(&parser, sb.ptr, sb.len, tokens, max_tokens);

	if(json_count < 0){
		struct discord_interaction_callback_data response = {
			.content = "Json parsing failed"
		};

		interaction_reply(response, client, event);

		sb_reset(&sb);
		return;
	}

	struct discord_embed embed = {
		.image = &(struct discord_embed_image){},
		.fields = &(struct discord_embed_fields){
			.array = &(struct discord_embed_field){
				.name = "favorites"
			},
			.size = 1
		}
	};

	for(int i = 0; i < json_count; ++i){
		struct string_view field = json_get(sb.ptr, tokens[i]);

		if(sv_equal(field, sv("url"))){
			embed.url = sv_save(json_get(sb.ptr, tokens[i + 1]));
		}else if(sv_equal(field, sv("jpg"))){
			embed.image->url = sv_save(json_get(sb.ptr, tokens[i + 3]));
		}else if(sv_equal(field, sv("name")))
			embed.title = sv_save(json_get(sb.ptr, tokens[i + 1]));
		else if(sv_equal(field, sv("about")))
			embed.description = sv_save(json_get(sb.ptr, tokens[i + 1]));
		else if(sv_equal(field, sv("favorites")))
			embed.fields->array[0].value = sv_save(json_get(sb.ptr, tokens[i + 1]));
	}

	struct discord_interaction_callback_data response = {
		.content = "",
		.embeds = &(struct discord_embeds){
			.array = &embed,
			.size = 1
		}
	};

	string_remove(embed.url, '\\');
	string_remove(embed.image->url, '\\');
	string_replace(embed.description, "\\n", "\n");

	interaction_reply(response, client, event);

	if(embed.url)
		free(embed.url);
	if(embed.image->url)
		free(embed.image->url);
	if(embed.title)
		free(embed.title);
	if(embed.description)
		free(embed.description);

	if(embed.fields->array[0].value)
		free(embed.fields->array[0].value);
	else
		embed.fields->array[0].value = "0";

	sb_reset(&sb);
}
