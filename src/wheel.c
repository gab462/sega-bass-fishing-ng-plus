void
wheel(struct discord *client, const struct discord_interaction *event)
{
	struct memory_arena arena = {};

	struct { struct discord_application_command_interaction_data_option *ptr; int len, cap; } options = {
		.ptr = event->data->options->array,
		.len = event->data->options->size
	};

	struct { struct string_view *ptr; int len, cap; } choices = {};

	da_for(option, options){
		if(strcmp(option->name, "choices") == 0){
			choices.ptr = ma_sv_split(&arena, sv(option->value), ",", &choices.len);
			choices.cap = choices.len;
		}
	}

	if(choices.len < 3){
		struct discord_interaction_callback_data response = {
			.content = "At least 3 choices required"
		};

		interaction_reply(response, client, event);

		ma_free(arena);
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
	vec2s font_size = {
		olivec_default_font.width * text_size,
		olivec_default_font.height * text_size,
	};

	MsfGifState state;
	msf_gif_begin(&state, width, height);

	Olivec_Canvas canvas = { (uint32_t *) pixels, width, height, width };

	srand(time(NULL));

	struct { uint32_t *ptr; int len, cap; } colors = {};

	ma_da_reserve(&arena, &colors, sectors);

	for(int i = 0; i < sectors; ++i)
		da_push(&colors, random_color());

	float angle = random_float() * 2.0 * GLM_PI;
	float sector_angle = (2.0 * GLM_PI) / sectors;
	float speed = .5f + random_float() * .5f;
	float accel = -.005f - random_float() * .01f;

	vec2s center = { width/2.f, height/2.f };

	for(int i = 0; i < 200; ++i){
		olivec_fill(canvas, 0xFFFFFFFF);

		angle += speed;
		speed += accel;

		if(speed <= 0.f)
			speed = 0.f;

		for(int i = 0; i < sectors; ++i){
			vec2s b = { width, height/2.f };
			vec2s c = { width, height/2.f };

			b = glms_vec2_sub(b, center);
			b = glms_vec2_rotate(b, angle + sector_angle * i);
			b = glms_vec2_add(b, center);

			c = glms_vec2_sub(c, center);
			c = glms_vec2_rotate(c, angle + sector_angle * (i + 1));
			c = glms_vec2_add(c, center);

			vec2s text_pos = b;

			vec2s middle = c;
			middle = glms_vec2_sub(middle, b);
			middle = glms_vec2_divs(middle, 2.f);

			text_pos = glms_vec2_add(text_pos, middle);

			vec2s font_middle = font_size;
			font_middle = glms_vec2_divs(font_middle, 2.f);

			text_pos = glms_vec2_sub(text_pos, font_middle);

			/* Closer to center */
			vec2s closer = text_pos;
			closer = glms_vec2_sub(closer, center);
			closer = glms_vec2_divs(closer, 8.f);

			text_pos = glms_vec2_sub(text_pos, closer);

			char text[2] = { 'a' + i, '\0' };

			olivec_triangle(canvas, center.x, center.y, b.x, b.y, c.x, c.y, colors.ptr[i]);
			olivec_text(canvas, text, text_pos.x, text_pos.y, olivec_default_font, text_size, 0xFF000000);
		}

		olivec_triangle(canvas, width/2.f - width/20.f, 0.f, width/2.f + width/20.f, 0.f, width/2.f, height/20.f, 0xFF000000);

		msf_gif_frame(&state, (uint8_t *) canvas.pixels, dur_cs, bit_depth, pitch_bytes);
	}

	MsfGifResult result = msf_gif_end(&state);

	struct string_view description = { .ptr = arena.end, .len = 0 };

	da_for(choice, choices){
		char text[2] = { 'a' + ((int) (choice - choices.ptr)), '\0' };

		memcpy(description.ptr + description.len, text, 1);
		description.len += 1;
		memcpy(description.ptr + description.len, " - ", 3);
		description.len += 3;
		memcpy(description.ptr + description.len, choice->ptr, choice->len);
		description.len += choice->len;
		memcpy(description.ptr + description.len, "\n", 1);
		description.len += 1;
	}

	description.ptr[description.len++] = '\0';

	/* Register bytes as used */
	ma_allocate_n(&arena, char, description.len);

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

	ma_free(arena);
	msf_gif_free(result);
}
