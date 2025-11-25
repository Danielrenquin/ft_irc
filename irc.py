#!/usr/bin/env python3
"""
Script pour lancer automatiquement le serveur IRC et 2 clients de test
Usage: python3 test_irc.py
"""

import subprocess
import time
import os

# Configuration
PORT = "6667"
PASSWORD = "42"
SERVER_CMD = f"valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes ./ircserv {PORT} {PASSWORD}"

# Commandes pour les clients (avec délais entre chaque commande)
CLIENT1_COMMANDS = [
    "PASS 42",
    "NICK Daniel",
    "USER Dan 0 * :Dan User"
]

CLIENT2_COMMANDS = [
    "PASS 42",
    "NICK Ln",
    "USER Ln 0 * :Ln User"
]

CLIENT3_COMMANDS = [
    "PASS 42",
    "NICK Lilian",
    "USER Lmonsat 0 * :Lmonsat User"
]

def detect_terminal():
    """Détecte le terminal disponible sur le système"""
    terminals = [
        ('gnome-terminal', ['gnome-terminal', '--']),
        ('konsole', ['konsole', '-e']),
        ('xfce4-terminal', ['xfce4-terminal', '-e']),
        ('xterm', ['xterm', '-e']),
        ('terminator', ['terminator', '-e']),
    ]
    
    for name, cmd in terminals:
        try:
            subprocess.run(['which', name], capture_output=True, check=True)
            return cmd
        except subprocess.CalledProcessError:
            continue
    
    return None

def launch_server(terminal_cmd):
    """Lance le serveur IRC dans un nouveau terminal"""
    print("🚀 Lancement du serveur IRC...")
    cmd = terminal_cmd + ['bash', '-c', f'{SERVER_CMD}; read -p "Appuyez sur Entrée pour fermer..."']
    subprocess.Popen(cmd)
    time.sleep(2)  # Attendre que le serveur démarre

def launch_client(terminal_cmd, client_name, commands):
    """Lance un client IRC dans un nouveau terminal"""
    print(f"👤 Lancement du client {client_name}...")
    
    # Créer les commandes avec délais
    cmd_string = ""
    for cmd in commands:
        cmd_string += f"echo '{cmd}'\n"
        cmd_string += f"sleep 0.5\n"
    
    # Créer un script temporaire pour le client
    script = f"""#!/bin/bash
echo "=== Connexion en tant que {client_name} ==="
sleep 1
{{
{cmd_string}
echo ""
echo "✅ Client {client_name} connecté et identifié!"
echo "Vous pouvez maintenant taper des commandes IRC..."
echo ""
cat  # Garder la connexion ouverte pour input manuel
}} | nc localhost {PORT}
read -p "Appuyez sur Entrée pour fermer..."
"""
    
    cmd = terminal_cmd + ['bash', '-c', script]
    subprocess.Popen(cmd)
    time.sleep(1.5)

def main():
    """Fonction principale"""
    print("=" * 50)
    print("🔧 Script de test IRC automatisé")
    print("=" * 50)
    
    # Vérifier que le binaire existe
    if not os.path.exists('./ircserv'):
        print("❌ Erreur: ./ircserv n'existe pas!")
        print("   Compilez d'abord votre projet avec 'make'")
        return 1
    
    # Détecter le terminal
    terminal_cmd = detect_terminal()
    if not terminal_cmd:
        print("❌ Aucun terminal compatible détecté!")
        print("   Terminaux supportés: gnome-terminal, konsole, xfce4-terminal, xterm, terminator")
        return 1
    
    print(f"✅ Terminal détecté: {terminal_cmd[0]}")
    print()
    
    # Lancer le serveur
    launch_server(terminal_cmd)
    
    # Lancer les clients
    launch_client(terminal_cmd, "Daniel", CLIENT1_COMMANDS)
    launch_client(terminal_cmd, "Ln", CLIENT2_COMMANDS)
    launch_client(terminal_cmd, "Lilian", CLIENT3_COMMANDS)
    
    print()
    print("=" * 50)
    print("✅ Tous les terminaux sont lancés!")
    print("=" * 50)
    print()
    print("📝 Instructions:")
    print("   - Le serveur tourne sur le port", PORT)
    print("   - Client 1: Daniel")
    print("   - Client 2: Ln")
    print("   - Vous pouvez maintenant taper des commandes IRC")
    print("   - Fermez chaque terminal quand vous avez terminé")
    print()
    
    return 0

if __name__ == "__main__":
    exit(main())