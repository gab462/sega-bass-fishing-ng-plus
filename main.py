from nextcord import Interaction, SlashOption, File, Embed
from nextcord.ext.commands import Bot
from PIL import Image, ImageDraw, ImageFont
import requests
import numpy
from io import BytesIO
import json
import random
import math
import os

bot = Bot()


@bot.slash_command(description="Responds with pong")
async def ping(interaction: Interaction):
    await interaction.response.send_message("Pong")


@bot.slash_command(description="Get random character from MAL")
async def character(interaction: Interaction):
    res = requests.get('https://api.jikan.moe/v4/random/characters')

    if not res.ok:
        await interaction.response.send_message("Error on request")
        return

    data = json.loads(res.content)['data']
    character_embed = Embed(title=data['name'], url=data['url'], description=data['about'])
    character_embed.set_image(data['images']['jpg']['image_url'])
    character_embed.add_field(name='favorites', value=data['favorites'])

    await interaction.response.send_message("", embed=character_embed)


@bot.slash_command(description="Spin the wheel")
async def wheel(interaction: Interaction, choices: str = SlashOption(description="Choices to be on the wheel")):
    choice_list = choices.split(',')
    sectors = len(choice_list)

    if sectors < 2:
        await interaction.response.send_message("At least two choices required")
        return

    width = 400
    height = 400

    text_size = width / 10
    default_font = ImageFont.load_default(size=text_size)

    frames = 200

    colors = []
    for i in range(sectors):
        colors.append((random.randint(127, 255), random.randint(127, 255), random.randint(127, 255)))

    angle = random.random() * 2.0 * math.pi
    sector_angle = (2.0 * math.pi) / sectors
    speed = 0.5 + random.random() * 0.5;
    accel = -0.005 - random.random() * 0.01;

    images = []

    rotate = lambda p, a, c: (numpy.array([
        [numpy.cos(a), -numpy.sin(a)],
        [numpy.sin(a),  numpy.cos(a)]
    ]) @ (p - c)) + c

    vec2 = lambda x, y: numpy.array([x, y], dtype=float)
    center = vec2(width/2.0, height/2.0)

    for i in range(frames):
        image = Image.new("RGB", (width, height), "white")
        draw = ImageDraw.Draw(image)

        angle += speed
        speed += accel

        if speed <= 0.0:
            speed = 0.0

        for j in range(sectors):
            b = rotate(vec2(width, height/2.0), angle + sector_angle * j, center)
            c = rotate(vec2(width, height/2.0), angle + sector_angle * (j + 1), center)

            text_pos = b + (c - b) / 2.0 - vec2(text_size, text_size) / 2.0
            text_pos -= (text_pos - center) / 8.0

            draw.polygon([(center[0], center[1]), (b[0], b[1]), (c[0], c[1])], fill=colors[j])
            draw.text((text_pos[0], text_pos[1]), f'{chr(ord("a") + j)}', font=default_font, fill=0)

        draw.polygon([(width/2.0 - width/20.0, 0.0), (width/2.0 + width/20.0, 0.0), (width/2.0, height/20.0)], fill=0)

        images.append(image)

    img = BytesIO()
    images[0].save(img, format='GIF', save_all=True, append_images=images[1:], duration=40, loop=0)
    img.seek(0)

    wheel_gif = File(img, filename='wheel.gif')

    wheel_description = "\n".join([f'{chr(ord("a") + i)} - {choice_list[i]}' for i in range(sectors)])

    wheel_embed = Embed(description=wheel_description)
    wheel_embed.set_image('attachment://wheel.gif')

    await interaction.response.send_message("", embed=wheel_embed, file=wheel_gif)


def main():
    bot.run(os.getenv("TOKEN"))


if __name__ == '__main__':
    main()
