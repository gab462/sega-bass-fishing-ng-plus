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

	if(ret != CCORD_OK && ret != CCORD_PENDING)
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
json_get(char *buffer, jsmntok_t token)
{
	return((struct string_view){
		.ptr = buffer + token.start,
		.len = token.end - token.start
	});
}

void
string_remove(char *string, char c)
{
	int len = strlen(string);

	for(int i = 0; i < len; ++i){
		if(string[i] == c){
			memmove(string + i, string + i + 1, len - i);
			--i;
			--len;
		}
	};
}

void
string_replace(char *string, char *from, char *to)
{
	int len = strlen(string);

	struct string_view from_sv = sv(from);
	struct string_view to_sv = sv(to);

	/* TODO: implement resizing */
	if(from_sv.len < to_sv.len)
		return;

	for(int i = 0; i < len; ++i){
		if(sv_equal(sv_left(sv(string + i), from_sv.len), from_sv)){
			for(int j = 0; j < to_sv.len; ++j){
				string[i + j] = to_sv.ptr[j];
			}

			memmove(string + i + to_sv.len,
					string + i + from_sv.len,
					len - i - from_sv.len + 1); /* \0 */

			len += to_sv.len - from_sv.len;
		}
	}
}

size_t
http_request_write_cb(void *contents, size_t size, size_t nmemb, void *userp)
{
	struct string_buffer *res = userp;

	struct string_view data = {
		.ptr = contents,
		.len = size * nmemb
	};

	sb_append_sv(res, data);

	return(data.len);
}

struct string_view
http_request(struct memory_arena *arena, char *url)
{
	CURL *curl = curl_easy_init();
	struct string_buffer res = { .ptr = ma_allocate_n(arena, char, 0), .cap = INT_MAX };

	curl_easy_setopt(curl, CURLOPT_URL, url);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, http_request_write_cb);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, &res);
	curl_easy_setopt(curl, CURLOPT_USERAGENT, "libcurl-agent/1.0");

	CURLcode ret = curl_easy_perform(curl);

	curl_easy_cleanup(curl);

	if(ret != CURLE_OK){
		return((struct string_view){});
	}

	res.cap = res.len;
	/* Register bytes as used */
	ma_allocate_n(arena, char, res.len);

	return(sv_from_sb(res));
}
