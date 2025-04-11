import discord
from discord.ext import commands
import os
import psutil

intents = discord.Intents.default()
intents.message_content = True
bot = commands.Bot(command_prefix="!", intents=intents, help_command=None)

ALLOWED_COMMANDS = ['1', '4','5', '6']
INTERFACES = ['wls3', 'wlxd03745740364']
ALLOWED_USER_ID = 888817795490521128  # ID-ul permis pentru a folosi comenzile

@bot.event
async def on_ready():
    print(f"[BOT] Logged in as {bot.user}")
    # Setare rich presence
    await update_rich_presence()

@bot.event
async def on_message(message):
    if message.author.id != ALLOWED_USER_ID:
        return  # Nu permite comenzile altor utilizatori
    await bot.process_commands(message)

async def update_rich_presence():
    net_stats = psutil.net_io_counters(pernic=True)
    wls3_sent = wls3_recv = 0
    wlxd03745740364_sent = wlxd03745740364_recv = 0

    for iface in INTERFACES:
        if iface in net_stats:
            sent = net_stats[iface].bytes_sent / (1024 * 1024)  # MB
            recv = net_stats[iface].bytes_recv / (1024 * 1024)  # MB
            if iface == "wls3":
                wls3_sent = sent
                wls3_recv = recv
            elif iface == "wlxd03745740364":
                wlxd03745740364_sent = sent
                wlxd03745740364_recv = recv

    # Setare rich presence
    await bot.change_presence(activity=discord.Activity(
        type=discord.ActivityType.playing,
        name=f"wls3: {wls3_sent + wls3_recv:.2f}MB | wlxd03745740364: {wlxd03745740364_sent + wlxd03745740364_recv:.2f}MB"
    ))

@bot.command()
async def cmd(ctx, arg):
    if ctx.author.id != ALLOWED_USER_ID:
        await ctx.send("❌ Nu ai permisiunea de a executa această comandă.")
        return

    if arg in ALLOWED_COMMANDS:
        with open("command_input.txt", "w") as f:
            f.write(arg)
        await ctx.send(f"✅ Comandă `{arg}` trimisă către aplicație.")
    else:
        await ctx.send("❌ Comandă invalidă. Folosește una dintre: `1`, `2`, `3`, `4`, `6`.")

@bot.command()
async def status(ctx):
    if ctx.author.id != ALLOWED_USER_ID:
        await ctx.send("❌ Nu ai permisiunea de a executa această comandă.")
        return

    usage_report = ""
    net_stats = psutil.net_io_counters(pernic=True)
    found = False

    for iface in INTERFACES:
        if iface in net_stats:
            found = True
            sent = net_stats[iface].bytes_sent / (1024 * 1024)  # MB
            recv = net_stats[iface].bytes_recv / (1024 * 1024)  # MB
            usage_report += f"📡 Interfață **{iface}** este **ONLINE**\n"
            usage_report += f"📤 Trimis: `{sent:.2f} MB` | 📥 Primit: `{recv:.2f} MB`\n\n"

    if not found:
        usage_report = "❌ Nicio interfață activă dintre `wls3` și `wlxd03745740364`."

    await ctx.send(usage_report)

@bot.command()
async def help(ctx):
    if ctx.author.id != ALLOWED_USER_ID:
        await ctx.send("❌ Nu ai permisiunea de a executa această comandă.")
        return

    help_msg = (
        "**📖 Comenzi disponibile:**\n"
        "`!cmd 1` - Reset toate sesiunile\n"
        "`!cmd 4` - Start/Stop code-server\n"
        "`!cmd 5` - Reset minecraftserver\n"
        "`!cmd 6` - Reset Playit\n"
        "`!status` - Afișează starea interfeței de rețea + trafic\n"
        "`!help` - Afișează acest mesaj de ajutor"
    )
    await ctx.send(help_msg)

bot.run("HELL NAAAA ;))")
