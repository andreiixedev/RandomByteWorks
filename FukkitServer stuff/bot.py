import discord
from discord.ext import commands, tasks
import os
import psutil
import platform
import subprocess
import time
from datetime import datetime, timedelta

# --- Configurație Bot ---
intents = discord.Intents.default()
intents.message_content = True
bot = commands.Bot(command_prefix="!", intents=intents, help_command=None)

ALLOWED_COMMANDS = ['1', '4', '5', '6']
INTERFACE = 'wls3'
TMUX_SESSIONS = ['minecraftserver', 'code-server', 'playit']
ALLOWED_USER_ID = 888817795490521128
START_TIME = time.time()

status_message = None
status_channel_id = None
status_end_time = None

# --- Task pentru actualizare status manual pentru 5 minute ---
@tasks.loop(seconds=30)
async def auto_status():
    global status_message, status_end_time
    if datetime.utcnow() > status_end_time:
        auto_status.stop()
        return
    channel = bot.get_channel(status_channel_id)
    if channel is None:
        return
    embed = await build_status_embed()
    try:
        if status_message:
            await status_message.edit(embed=embed)
        else:
            status_message = await channel.send(embed=embed)
    except discord.HTTPException:
        status_message = await channel.send(embed=embed)

# --- Construire embed Status ---
async def build_status_embed():
    uptime_seconds = int(time.time() - START_TIME)
    uptime_str = str(timedelta(seconds=uptime_seconds))
    cpu_count = psutil.cpu_count(logical=False)
    cpu_threads = psutil.cpu_count(logical=True)
    cpu_freq = psutil.cpu_freq().current if psutil.cpu_freq() else 0
    cpu_percent = psutil.cpu_percent(interval=None)
    mem = psutil.virtual_memory()
    total_mem = mem.total / (1024**3)
    used_mem = mem.used / (1024**3)
    available_mem = mem.available / (1024**3)
    ram_percent = mem.percent
    net_stats = psutil.net_io_counters(pernic=True)
    if INTERFACE in net_stats:
        sent = net_stats[INTERFACE].bytes_sent / (1024 * 1024)
        recv = net_stats[INTERFACE].bytes_recv / (1024 * 1024)
        net_status = "ONLINE"
    else:
        sent = recv = 0.0
        net_status = "OFFLINE"
    session_status = []
    for s in TMUX_SESSIONS:
        res = subprocess.run(["tmux", "has-session", "-t", s], capture_output=True)
        state = "✅ Running" if res.returncode == 0 else "❌ Stopped"
        session_status.append(f"**{s}**: {state}")
    sessions_str = "\n".join(session_status)
    embed = discord.Embed(title="📊 System Status", color=0x3498db, timestamp=datetime.utcnow())
    embed.add_field(name="🕒 Uptime", value=uptime_str, inline=False)
    embed.add_field(name="💻 CPU", value=(
        f"Model: {platform.processor()}\n"
        f"Cores: {cpu_count} ({cpu_threads} threads)\n"
        f"Freq: {cpu_freq:.2f}MHz\n"
        f"Usage: {cpu_percent:.1f}%"
    ), inline=False)
    embed.add_field(name="📀 RAM", value=(
        f"Total: {total_mem:.2f} GB\n"
        f"Used: {used_mem:.2f} GB ({ram_percent:.1f}%)\n"
        f"Free: {available_mem:.2f} GB"
    ), inline=False)
    embed.add_field(name=f"📱 Network ({INTERFACE})", value=(
        f"Status: {net_status}\n"
        f"Sent: {sent:.2f} MB\n"
        f"Recv: {recv:.2f} MB"
    ), inline=False)
    embed.add_field(name="🔧 TMUX Sessions", value=sessions_str, inline=False)
    embed.set_footer(text="Fukkit Server Panel v1.0")
    embed.set_image(url="https://media.tenor.com/tmwsleNDQ5cAAAAM/noko-kanoko-anime.gif")
    return embed

# --- Evenimente & Comenzi Bot ---
@bot.event
async def on_ready():
    print(f"[BOT] Logged in as {bot.user}")

@bot.event
async def on_message(message):
    if message.author.id != ALLOWED_USER_ID:
        return
    await bot.process_commands(message)

@bot.command()
async def cmd(ctx, arg):
    embed = discord.Embed(title="🛠️ Execute Command", color=0x00ff00)
    if arg in ALLOWED_COMMANDS:
        with open("command_input.txt", "w") as f:
            f.write(arg)
        embed.description = f"✅ Comandă `{arg}` trimisă către FUKKIT SERVER DASHBOARD."
        embed.set_image(url="https://i.pinimg.com/originals/a0/09/71/a00971f1d12be1540029266530a8c1b7.gif")
    else:
        embed.color = 0xff0000
        embed.description = "❌ Comandă invalidă. Folosește: `1`, `4`, `5`, `6`."
        embed.set_image(url="https://media.tenor.com/nIfKxqBUqQQAAAAM/shake-head-anime.gif")
    await ctx.send(embed=embed)

@bot.command()
async def status(ctx):
    global status_message, status_channel_id, status_end_time
    status_channel_id = ctx.channel.id
    status_message = await ctx.send(embed=await build_status_embed())
    status_end_time = datetime.utcnow() + timedelta(minutes=5)
    if not auto_status.is_running():
        auto_status.start()

@bot.command()
async def help(ctx):
    embed = discord.Embed(title="📖 Ajutor Comenzi", color=0x95a5a6)
    embed.add_field(name="!cmd 1", value="Execută comanda 'Reset all sessions'", inline=False)
    embed.add_field(name="!cmd 4", value="Execută comanda 'Start/Stop Code-Server'", inline=False)
    embed.add_field(name="!cmd 5", value="Execută comanda 'Reset Minecraft Server'", inline=False)
    embed.add_field(name="!cmd 6", value="Execută comanda 'Reset Playit'", inline=False)

    embed.add_field(name="!status", value="Afișează starea sistemului pentru 5 minute", inline=False)
    embed.add_field(name="!help", value="Afișează acest mesaj", inline=False)
    embed.set_image(url="https://www.gifcen.com/wp-content/uploads/2022/01/anime-gif-9.gif")
    await ctx.send(embed=embed)

@tasks.loop(seconds=30)
async def update_rich_presence():
    mem = psutil.virtual_memory()
    available_mem = mem.available / (1024 ** 3)
    cpu_percent = psutil.cpu_percent(interval=None)
    latency = round(bot.latency * 1000, 2)

    net_stats = psutil.net_io_counters(pernic=True)
    sent = recv = 0.0
    if INTERFACE in net_stats:
        sent = net_stats[INTERFACE].bytes_sent / (1024 * 1024)
        recv = net_stats[INTERFACE].bytes_recv / (1024 * 1024)
    total_net = sent + recv

    activity = discord.Activity(
        type=discord.ActivityType.watching,
        name=(
            f"CPU: {cpu_percent:.0f}% | "
            f"Ping: {latency}ms | "
            f"Net: {total_net:.1f}MB"
        )
    )
    await bot.change_presence(activity=activity)

@bot.event
async def on_ready():
    print(f"[BOT] Logged in as {bot.user}")
    update_rich_presence.start()

# --- Pornire Bot ---
bot.run(os.getenv("DISCORD_BOT_TOKEN"))