#include <concord/discord.h>
#include <cut.c>
#include <time.h>
#include <stdlib.h>
#include <math.h>
#include <cglm.h>
#include <curl/curl.h>
#include <concord/jsmn.h>
#define OLIVEC_IMPLEMENTATION
#include <olive.c>
#define MSF_GIF_IMPL
#include <msf_gif.h>

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

void
pong(struct discord *client, const struct discord_interaction *event)
{
	struct discord_interaction_callback_data response = {
		.content = "pong"
	};

	interaction_reply(response, client, event);
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
random_float()
{
	return(rand() / (float) RAND_MAX);
}

void
wheel(struct discord *client, const struct discord_interaction *event)
{
	struct { struct discord_application_command_interaction_data_option *ptr; int len, cap; } options = {
		.ptr = event->data->options->array,
		.len = event->data->options->size
	};

	struct { struct string_view *ptr; int len, cap; } choices = {};

	da_for(option, options){
		if(strcmp(option->name, "choices") == 0){
			choices.ptr = sv_split(sv(option->value), ",", &choices.len);
			choices.cap = choices.len;
		}
	}

	if(choices.len < 3){
		struct discord_interaction_callback_data response = {
			.content = "At least 3 choices required"
		};

		interaction_reply(response, client, event);

		da_reset(&choices);
		return;
	}

	constexpr int width = 400;
	constexpr int height = 400;

	int dur_cs = 4;
	int bit_depth = 16;
	int pitch_bytes = width * 4;
	int sectors = choices.len;

	uint32_t pixels[height][width] = {};

	int text_size = width / 100;
	vec4 font_size = {
		olivec_default_font.width * text_size,
		olivec_default_font.height * text_size,
	};

	MsfGifState state;
	msf_gif_begin(&state, width, height);

	Olivec_Canvas canvas = { (uint32_t *) pixels, width, height, width };

	srand(time(NULL));

	struct { uint32_t *ptr; int len, cap; } colors = {};

	for(int i = 0; i < sectors; ++i)
		da_push(&colors, random_color());

	float angle = random_float() * 2.0 * GLM_PI;
	float sector_angle = (2.0 * GLM_PI) / sectors;
	float speed = .5f + random_float() * .5f;
	float accel = -.005f - random_float() * .01f;

	vec4 center = { width/2.f, height/2.f };

	for(int i = 0; i < 200; ++i){
		olivec_fill(canvas, 0xFFFFFFFF);

		angle += speed;
		speed += accel;

		if(speed <= 0.f)
			speed = 0.f;

		for(int i = 0; i < sectors; ++i){
			vec4 b = { width, height/2.f };
			vec4 c = { width, height/2.f };

			glm_vec2_sub(b, center, b);
			glm_vec2_rotate(b, angle + sector_angle * i, b);
			glm_vec2_add(b, center, b);

			glm_vec2_sub(c, center, c);
			glm_vec2_rotate(c, angle + sector_angle * (i + 1), c);
			glm_vec2_add(c, center, c);

			vec4 text_pos = {};
			glm_vec2_add(text_pos, b, text_pos);

			vec4 middle = {};
			glm_vec2_add(middle, c, middle);
			glm_vec2_sub(middle, b, middle);
			glm_vec2_divs(middle, 2.f, middle);

			glm_vec2_add(text_pos, middle, text_pos);

			vec4 font_middle = {};
			glm_vec2_add(font_middle, font_size, font_middle);
			glm_vec2_divs(font_middle, 2.f, font_middle);

			glm_vec2_sub(text_pos, font_middle, text_pos);

			/* Closer to center */
			vec4 closer = {};
			glm_vec2_add(closer, text_pos, closer);
			glm_vec2_sub(closer, center, closer);
			glm_vec2_divs(closer, 8.f, closer);

			glm_vec2_sub(text_pos, closer, text_pos);

			char text[2] = { 'a' + i, '\0' };

			olivec_triangle(canvas, center[0], center[1], b[0], b[1], c[0], c[1], colors.ptr[i]);
			olivec_text(canvas, text, text_pos[0], text_pos[1], olivec_default_font, text_size, 0xFF000000);
		}

		olivec_triangle(canvas, width/2.f - width/20.f, 0.f, width/2.f + width/20.f, 0.f, width/2.f, height/20.f, 0xFF000000);

		msf_gif_frame(&state, (uint8_t *) canvas.pixels, dur_cs, bit_depth, pitch_bytes);
	}

	MsfGifResult result = msf_gif_end(&state);

	struct string_buffer description = {};

	for(int i = 0; i < choices.len; ++i){
		char text[2] = { 'a' + i, '\0' };

		sb_append(&description, text);
		sb_append(&description, " - ");
		sb_append_sv(&description, choices.ptr[i]);
		sb_append(&description, "\n");
	}

	sb_terminate(&description);

	struct discord_attachments attachments = {
		.array = (struct discord_attachment[]){
			{
				.filename = "wheel.gif",
				.content = (char *) result.data,
				.size = result.dataSize
			}
		},
		.size = 1
	};

	struct discord_embeds embeds = {
		.array = (struct discord_embed[]){
			{
				.image = &(struct discord_embed_image){
					.url = "attachment://wheel.gif"
				},
				.description = description.ptr
			}
		},
		.size = 1
	};

	struct discord_interaction_callback_data response = {
		.content = "",
		.attachments = &attachments,
		.embeds = &embeds
	};

	interaction_reply(response, client, event);

	da_reset(&choices);
	da_reset(&colors);
	sb_reset(&description);
	msf_gif_free(result);
}

size_t
write_cb(void *contents, size_t size, size_t nmemb, void *userp)
{
	struct string_buffer *sb = userp;
	struct string_view data = {
		.ptr = contents,
		.len = size * nmemb
	};

	sb_append_sv(sb, data);

	return size * nmemb;
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

void
character(struct discord *client, const struct discord_interaction *event)
{
	CURL *curl = curl_easy_init();
	struct string_buffer sb = {};
	char *url = "https://api.jikan.moe/v4/random/characters";

	curl_easy_setopt(curl, CURLOPT_URL, url);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_cb);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, &sb);
	curl_easy_setopt(curl, CURLOPT_USERAGENT, "libcurl-agent/1.0");

	CURLcode ret = curl_easy_perform(curl);

	if(ret != CURLE_OK){
		struct discord_interaction_callback_data response = {
			.content = "Request failed"
		};

		interaction_reply(response, client, event);

		sb_reset(&sb);
		curl_easy_cleanup(curl);
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
		curl_easy_cleanup(curl);
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
		struct string_view field = get_json_token(sb.ptr, tokens[i]);

		if(sv_equal(field, sv("url"))){
			embed.url = sv_save(get_json_token(sb.ptr, tokens[i + 1]));
		}else if(sv_equal(field, sv("jpg"))){
			embed.image->url = sv_save(get_json_token(sb.ptr, tokens[i + 3]));
		}else if(sv_equal(field, sv("name")))
			embed.title = sv_save(get_json_token(sb.ptr, tokens[i + 1]));
		else if(sv_equal(field, sv("about")))
			embed.description = sv_save(get_json_token(sb.ptr, tokens[i + 1]));
		else if(sv_equal(field, sv("favorites")))
			embed.fields->array[0].value = sv_save(get_json_token(sb.ptr, tokens[i + 1]));
	}

	struct discord_interaction_callback_data response = {
		.content = "",
		.embeds = &(struct discord_embeds){
			.array = &embed,
			.size = 1
		}
	};

	url_remove_backslashes(embed.url);
	url_remove_backslashes(embed.image->url);
	parse_newlines(embed.description);

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

	curl_easy_cleanup(curl);
	sb_reset(&sb);
}

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
