void
mal_character(struct memory_arena *arena,
	      struct discord *client, const struct discord_interaction *event)
{
	struct string_view res = http_request(arena, "https://api.jikan.moe/v4/random/characters");

	if(res.start == nullptr){
		struct discord_interaction_callback_data response = {
			.content = "Request failed"
		};

		interaction_reply(response, client, event);
		return;
	}

	res.start[res.len++] = '\0';
	ma_allocate(arena, char);

	constexpr int max_tokens = 64;

	jsmn_parser parser;
	jsmntok_t tokens[max_tokens];

	jsmn_init(&parser);

	int json_count = jsmn_parse(&parser, res.start, res.len, tokens, max_tokens);

	if(json_count < 0){
		struct discord_interaction_callback_data response = {
			.content = "Json parsing failed"
		};

		interaction_reply(response, client, event);
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
		struct string_view field = json_get(res.start, tokens[i]);

		if(sv_equal(field, sv("url")))
			embed.url = ma_sv_save(arena, json_get(res.start, tokens[i + 1]));
		else if(sv_equal(field, sv("jpg")))
			embed.image->url = ma_sv_save(arena, json_get(res.start, tokens[i + 3]));
		else if(sv_equal(field, sv("name")))
			embed.title = ma_sv_save(arena, json_get(res.start, tokens[i + 1]));
		else if(sv_equal(field, sv("about")))
			embed.description = ma_sv_save(arena, json_get(res.start, tokens[i + 1]));
		else if(sv_equal(field, sv("favorites")))
			embed.fields->array[0].value = ma_sv_save(arena, json_get(res.start, tokens[i + 1]));
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
}
