void
wheel(struct memory_arena *arena,
      struct discord *client, const struct discord_interaction *event)
{
	if(event->data->options->size != 1 || strcmp(event->data->options->array[0].name, "choices") != 0){
		struct discord_interaction_callback_data response = {
			.content = "Error when parsing options"
		};

		interaction_reply(response, client, event);
		return;

	}

	struct string_view *choices = ma_sv_split(arena, sv(event->data->options->array[0].value), ",");

	if(da_len(choices) < 3){
		struct discord_interaction_callback_data response = {
			.content = "At least 3 choices required"
		};

		interaction_reply(response, client, event);
		return;
	}

	constexpr int width = 400;
	constexpr int height = 400;

	int dur_cs = 4;
	int bit_depth = 16;
	int pitch_bytes = width * 4;
	int sectors = da_len(choices);

	uint32_t pixels[height][width] = {};

	int text_size = width / 100;
	vec2s font_size = {
		olivec_default_font.width * text_size,
		olivec_default_font.height * text_size,
	};

	MsfGifState state = {
		.customAllocatorContext = arena
	};

	msf_gif_begin(&state, width, height);

	Olivec_Canvas canvas = { (uint32_t *) pixels, width, height, width };

	srand(time(NULL));

	uint32_t *colors = nullptr;

	ma_da_reserve(arena, &colors, sectors);

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

			olivec_triangle(canvas, center.x, center.y, b.x, b.y, c.x, c.y, colors[i]);
			olivec_text(canvas, text, text_pos.x, text_pos.y, olivec_default_font, text_size, 0xFF000000);
		}

		olivec_triangle(canvas, width/2.f - width/20.f, 0.f, width/2.f + width/20.f, 0.f, width/2.f, height/20.f, 0xFF000000);

		msf_gif_frame(&state, (uint8_t *) canvas.pixels, dur_cs, bit_depth, pitch_bytes);
	}

	MsfGifResult result = msf_gif_end(&state);

	char *description = nullptr;
	ma_sb_reserve(arena, &description, 0);
	da_header(description)->cap = INT_MAX;

	da_for(choice, choices){
		da_push(&description, 'a' + ((int) (choice - choices)));
		sb_append(&description, " - ");
		sb_append_sv(&description, *choice);
		sb_append(&description, "\n");
	}

	sb_terminate(&description);

	da_header(description)->cap = da_header(description)->len;

	/* Register bytes as used */
	ma_allocate_n(arena, char, da_len(description));

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
				.description = description
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
}
