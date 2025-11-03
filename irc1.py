#!/usr/bin/env python3
"""
Script de test IRC amélioré avec tests automatisés KICK
Usage: python3 test_irc.py [options]
"""
import subprocess
import time
import os
import sys
import argparse
import socket
import threading

# Configuration
PORT = "6667"
PASSWORD = "42"
SERVER_CMD = f"./ircserv {PORT} {PASSWORD}"

# Commandes de base pour les clients
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


# ========================================
# DÉTECTION TERMINAL
# ========================================

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


# ========================================
# LANCEMENT SERVEUR ET CLIENTS
# ========================================

def launch_server(terminal_cmd):
    """Lance le serveur IRC dans un nouveau terminal"""
    print("🚀 Lancement du serveur IRC...")
    cmd = terminal_cmd + ['bash', '-c', f'{SERVER_CMD}; read -p "Appuyez sur Entrée pour fermer..."']
    subprocess.Popen(cmd)
    time.sleep(2)  # Attendre que le serveur démarre


def launch_client(terminal_cmd, client_name, commands, extra_commands=None):
    """Lance un client IRC dans un nouveau terminal"""
    print(f"👤 Lancement du client {client_name}...")
    
    # Créer les commandes avec délais
    cmd_string = ""
    for cmd in commands:
        cmd_string += f"echo '{cmd}'\n"
        cmd_string += f"sleep 0.5\n"
    
    # Ajouter des commandes supplémentaires si fournies
    if extra_commands:
        cmd_string += "sleep 1\n"
        for cmd in extra_commands:
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


# ========================================
# TESTS AUTOMATISÉS KICK
# ========================================

class IRCTester:
    """Testeur automatisé pour les commandes IRC"""
    
    def __init__(self, port=6667, password="42"):
        self.port = port
        self.password = password
        self.clients = {}
    
    def connect_client(self, nickname):
        """Connecte un client et retourne le socket"""
        try:
            sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            sock.connect(('localhost', self.port))
            
            # Authentification
            sock.send(f"PASS {self.password}\r\n".encode())
            time.sleep(0.2)
            sock.send(f"NICK {nickname}\r\n".encode())
            time.sleep(0.2)
            sock.send(f"USER {nickname} 0 * :{nickname}\r\n".encode())
            time.sleep(0.5)
            
            # Lire la réponse
            sock.settimeout(1.0)
            try:
                response = sock.recv(4096).decode('utf-8', errors='ignore')
                print(f"[{nickname}] Réponse connexion: {response[:100]}...")
            except socket.timeout:
                pass
            
            self.clients[nickname] = sock
            return sock
            
        except Exception as e:
            print(f"❌ Erreur connexion {nickname}: {e}")
            return None
    
    def send_command(self, nickname, command):
        """Envoie une commande et lit la réponse"""
        if nickname not in self.clients:
            print(f"❌ Client {nickname} non connecté")
            return None
        
        sock = self.clients[nickname]
        try:
            print(f"[{nickname}] >>> {command}")
            sock.send(f"{command}\r\n".encode())
            time.sleep(0.3)
            
            sock.settimeout(0.5)
            try:
                response = sock.recv(4096).decode('utf-8', errors='ignore')
                if response:
                    print(f"[{nickname}] <<< {response.strip()}")
                return response
            except socket.timeout:
                return None
                
        except Exception as e:
            print(f"❌ Erreur envoi commande: {e}")
            return None
    
    def disconnect_client(self, nickname):
        """Déconnecte un client"""
        if nickname in self.clients:
            try:
                self.clients[nickname].send(b"QUIT :Goodbye\r\n")
                self.clients[nickname].close()
            except:
                pass
            del self.clients[nickname]
    
    def disconnect_all(self):
        """Déconnecte tous les clients"""
        for nick in list(self.clients.keys()):
            self.disconnect_client(nick)
    
    def test_kick_basic(self):
        """Test 1: KICK basique"""
        print("\n" + "="*60)
        print("TEST 1: KICK BASIQUE")
        print("="*60)
        
        # Connexion
        self.connect_client("Alice")
        self.connect_client("Bob")
        
        # Rejoindre un canal
        self.send_command("Alice", "JOIN #test")
        self.send_command("Bob", "JOIN #test")
        time.sleep(0.5)
        
        # Alice kick Bob
        print("\n➡️  Alice kick Bob du canal #test")
        self.send_command("Alice", "KICK #test Bob")
        time.sleep(0.5)
        
        # Vérifier que Bob ne peut plus envoyer de messages
        print("\n➡️  Bob essaie d'envoyer un message (devrait échouer)")
        self.send_command("Bob", "PRIVMSG #test :Je suis encore là?")
        
        time.sleep(1)
    
    def test_kick_with_reason(self):
        """Test 2: KICK avec raison"""
        print("\n" + "="*60)
        print("TEST 2: KICK AVEC RAISON")
        print("="*60)
        
        self.send_command("Alice", "JOIN #test2")
        self.send_command("Bob", "JOIN #test2")
        time.sleep(0.5)
        
        print("\n➡️  Alice kick Bob avec raison détaillée")
        self.send_command("Alice", "KICK #test2 Bob :Comportement inapproprié")
        time.sleep(1)
    
    def test_kick_no_permission(self):
        """Test 3: KICK sans permission"""
        print("\n" + "="*60)
        print("TEST 3: KICK SANS PERMISSION (devrait échouer)")
        print("="*60)
        
        self.connect_client("Charlie")
        
        self.send_command("Alice", "JOIN #test3")
        self.send_command("Charlie", "JOIN #test3")
        time.sleep(0.5)
        
        print("\n➡️  Charlie (non-op) tente de kick Alice")
        response = self.send_command("Charlie", "KICK #test3 Alice")
        
        if response and "482" in response:
            print("✅ Erreur 482 ERR_CHANOPRIVSNEEDED reçue correctement")
        else:
            print("⚠️  Erreur attendue: 482 ERR_CHANOPRIVSNEEDED")
        
        time.sleep(1)
    
    def test_kick_nonexistent_channel(self):
        """Test 4: KICK sur canal inexistant"""
        print("\n" + "="*60)
        print("TEST 4: KICK SUR CANAL INEXISTANT (devrait échouer)")
        print("="*60)
        
        print("\n➡️  Alice tente de kick Bob d'un canal inexistant")
        response = self.send_command("Alice", "KICK #nonexistent Bob")
        
        if response and "403" in response:
            print("✅ Erreur 403 ERR_NOSUCHCHANNEL reçue correctement")
        else:
            print("⚠️  Erreur attendue: 403 ERR_NOSUCHCHANNEL")
        
        time.sleep(1)
    
    def test_kick_not_on_channel(self):
        """Test 5: KICK quand pas sur le canal"""
        print("\n" + "="*60)
        print("TEST 5: KICK SANS ÊTRE SUR LE CANAL (devrait échouer)")
        print("="*60)
        
        self.send_command("Bob", "JOIN #test5")
        time.sleep(0.5)
        
        print("\n➡️  Alice (pas sur #test5) tente de kick Bob")
        response = self.send_command("Alice", "KICK #test5 Bob")
        
        if response and "442" in response:
            print("✅ Erreur 442 ERR_NOTONCHANNEL reçue correctement")
        else:
            print("⚠️  Erreur attendue: 442 ERR_NOTONCHANNEL")
        
        time.sleep(1)
    
    def test_kick_missing_params(self):
        """Test 6: KICK avec paramètres manquants"""
        print("\n" + "="*60)
        print("TEST 6: KICK PARAMÈTRES MANQUANTS (devrait échouer)")
        print("="*60)
        
        print("\n➡️  KICK sans paramètres")
        response = self.send_command("Alice", "KICK")
        if response and "461" in response:
            print("✅ Erreur 461 ERR_NEEDMOREPARAMS reçue")
        
        time.sleep(0.5)
        
        print("\n➡️  KICK avec un seul paramètre")
        response = self.send_command("Alice", "KICK #test")
        if response and "461" in response:
            print("✅ Erreur 461 ERR_NEEDMOREPARAMS reçue")
        
        time.sleep(1)
    
    def run_all_tests(self):
        """Lance tous les tests automatisés"""
        print("\n" + "="*60)
        print("🧪 LANCEMENT DES TESTS AUTOMATISÉS KICK")
        print("="*60)
        
        try:
            self.test_kick_basic()
            self.test_kick_with_reason()
            self.test_kick_no_permission()
            self.test_kick_nonexistent_channel()
            self.test_kick_not_on_channel()
            self.test_kick_missing_params()
            
            print("\n" + "="*60)
            print("✅ TOUS LES TESTS TERMINÉS")
            print("="*60)
            
        finally:
            print("\n🔌 Déconnexion des clients...")
            self.disconnect_all()


# ========================================
# SCÉNARIOS DE TEST PRÉDÉFINIS
# ========================================

def scenario_kick_tests(terminal_cmd):
    """Scénario: Tests manuels KICK avec 3 clients"""
    print("\n📋 Scénario: Tests manuels KICK")
    print("-" * 50)
    
    # Lancer le serveur
    launch_server(terminal_cmd)
    
    # Client 1: Alice (opératrice)
    alice_extra = [
        "JOIN #kicktest",
        "MODE #kicktest +o Alice"  # Se donner l'op si possible
    ]
    launch_client(terminal_cmd, "Alice", CLIENT1_COMMANDS, alice_extra)
    
    # Client 2: Bob (victime)
    bob_extra = [
        "JOIN #kicktest"
    ]
    launch_client(terminal_cmd, "Bob", CLIENT2_COMMANDS, bob_extra)
    
    # Client 3: Charlie (observateur)
    charlie_extra = [
        "JOIN #kicktest"
    ]
    launch_client(terminal_cmd, "Charlie", CLIENT3_COMMANDS, charlie_extra)
    
    print("\n✅ Scénario lancé!")
    print("\n📝 Commandes à tester dans le terminal Alice:")
    print("   KICK #kicktest Bob")
    print("   KICK #kicktest Bob :Raison du kick")
    print("   KICK #kicktest Charlie :Test avec raison")


def scenario_basic(terminal_cmd):
    """Scénario de base: 3 clients simples"""
    print("\n📋 Scénario: 3 clients basiques")
    print("-" * 50)
    
    launch_server(terminal_cmd)
    launch_client(terminal_cmd, "Daniel", CLIENT1_COMMANDS)
    launch_client(terminal_cmd, "Ln", CLIENT2_COMMANDS)
    launch_client(terminal_cmd, "Lilian", CLIENT3_COMMANDS)
    
    print("\n✅ Scénario de base lancé!")


def scenario_channel_ops(terminal_cmd):
    """Scénario: Tests opérateurs de canal"""
    print("\n📋 Scénario: Tests opérateurs")
    print("-" * 50)
    
    launch_server(terminal_cmd)
    
    # Client 1 avec commandes d'opérateur
    op_commands = [
        "JOIN #test",
        "MODE #test +o Daniel",
        "TOPIC #test :Canal de test"
    ]
    launch_client(terminal_cmd, "Daniel", CLIENT1_COMMANDS, op_commands)
    
    # Clients normaux
    launch_client(terminal_cmd, "Ln", CLIENT2_COMMANDS, ["JOIN #test"])
    launch_client(terminal_cmd, "Lilian", CLIENT3_COMMANDS, ["JOIN #test"])
    
    print("\n✅ Scénario opérateurs lancé!")


# ========================================
# MENU INTERACTIF
# ========================================

def print_menu():
    """Affiche le menu principal"""
    print("\n" + "="*60)
    print("🔧 TESTEUR IRC AMÉLIORÉ - MENU PRINCIPAL")
    print("="*60)
    print("\n📱 LANCEMENT MANUEL (avec terminaux)")
    print("  1. Lancer scénario basique (3 clients)")
    print("  2. Lancer scénario tests KICK (3 clients)")
    print("  3. Lancer scénario opérateurs de canal")
    print("  4. Lancer serveur uniquement")
    print("\n🤖 TESTS AUTOMATISÉS")
    print("  5. Tests automatiques KICK (tous)")
    print("  6. Test KICK basique")
    print("  7. Test KICK avec raison")
    print("  8. Test KICK sans permission")
    print("  9. Afficher checklist implémentation KICK")
    print("\n⚙️  CONFIGURATION")
    print("  c. Changer port/password")
    print("  i. Informations système")
    print("  q. Quitter")
    print("="*60)


def show_implementation_checklist():
    """Affiche la checklist d'implémentation"""
    print("""
="*60)
📋 CHECKLIST D'IMPLÉMENTATION KICK (RFC 1459)
="*60)

□ 1. PARSING DE LA COMMANDE
   □ Extraire le(s) canal/canaux
   □ Extraire le(s) utilisateur(s)
   □ Extraire le commentaire optionnel (après ':')
   □ Gérer les listes séparées par virgules

□ 2. VALIDATION
   □ Vérifier nombre minimum de paramètres (2)
   □ Valider format du nom de canal (#, &, !, +)
   □ Vérifier existence du canal
   □ Vérifier que l'émetteur est sur le canal
   □ Vérifier que l'émetteur est opérateur (@)
   □ Vérifier que l'utilisateur cible est sur le canal

□ 3. CODES D'ERREUR À IMPLÉMENTER
   □ 461 ERR_NEEDMOREPARAMS - pas assez de paramètres
   □ 403 ERR_NOSUCHCHANNEL - canal inexistant
   □ 476 ERR_BADCHANMASK - format canal invalide
   □ 442 ERR_NOTONCHANNEL - émetteur pas sur le canal
   □ 482 ERR_CHANOPRIVSNEEDED - pas opérateur
   □ 441 ERR_USERNOTINCHANNEL - utilisateur pas sur canal

□ 4. EXÉCUTION
   □ Retirer l'utilisateur du canal
   □ Envoyer notification KICK à tous les membres
      Format: :kicker!user@host KICK #channel victim :reason
   □ Mettre à jour la liste des membres
   □ Logger l'action

□ 5. EXTENSIONS (optionnelles)
   □ Support multi-canaux
   □ Support multi-utilisateurs
   □ Validation nombre canaux = nombre utilisateurs

□ 6. SÉCURITÉ
   □ Limiter longueur du commentaire (512 caractères max)
   □ Prévenir injection de commandes
   □ Rate limiting anti-abus
   □ Logger les tentatives non autorisées

✅ EXEMPLE DE MESSAGES ATTENDUS:
   Succès: :Alice!~Dan@host KICK #test Bob :Bad behavior
   Erreur: :server 482 Alice #test :You're not channel operator
""")


def show_system_info():
    """Affiche les informations système"""
    print("\n" + "="*60)
    print("ℹ️  INFORMATIONS SYSTÈME")
    print("="*60)
    
    # Vérifier serveur
    if os.path.exists('./ircserv'):
        print("✅ Serveur IRC: ./ircserv trouvé")
    else:
        print("❌ Serveur IRC: ./ircserv non trouvé")
    
    # Vérifier terminal
    terminal = detect_terminal()
    if terminal:
        print(f"✅ Terminal: {terminal[0]}")
    else:
        print("❌ Terminal: Aucun terminal compatible détecté")
    
    # Configuration actuelle
    print(f"\n⚙️  Configuration:")
    print(f"   Port: {PORT}")
    print(f"   Password: {PASSWORD}")
    print(f"   Commande serveur: {SERVER_CMD}")
    
    # Vérifier si le port est disponible
    try:
        test_sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        test_sock.settimeout(1)
        result = test_sock.connect_ex(('localhost', int(PORT)))
        test_sock.close()
        
        if result == 0:
            print(f"⚠️  Port {PORT}: Occupé (serveur déjà lancé?)")
        else:
            print(f"✅ Port {PORT}: Disponible")
    except:
        print(f"❓ Port {PORT}: État inconnu")
    
    print("="*60)


# ========================================
# FONCTION PRINCIPALE
# ========================================

def main():
    """Fonction principale"""
    global PORT, PASSWORD, SERVER_CMD
    
    # Parser les arguments
    parser = argparse.ArgumentParser(description='Testeur IRC amélioré')
    parser.add_argument('--auto-kick', action='store_true', help='Lance les tests KICK automatiquement')
    parser.add_argument('--port', type=str, default='6667', help='Port du serveur')
    parser.add_argument('--password', type=str, default='42', help='Password du serveur')
    args = parser.parse_args()
    
    PORT = args.port
    PASSWORD = args.password
    SERVER_CMD = f"./ircserv {PORT} {PASSWORD}"
    
    # Mode automatique
    if args.auto_kick:
        print("🤖 Mode automatique: Tests KICK")
        if not os.path.exists('./ircserv'):
            print("❌ Erreur: ./ircserv n'existe pas!")
            return 1
        
        # Lancer le serveur
        print("🚀 Lancement du serveur...")
        server_proc = subprocess.Popen([SERVER_CMD], shell=True)
        time.sleep(2)
        
        try:
            # Lancer les tests
            tester = IRCTester(int(PORT), PASSWORD)
            tester.run_all_tests()
        finally:
            print("\n🛑 Arrêt du serveur...")
            server_proc.terminate()
        
        return 0
    
    # Mode interactif
    print("=" * 60)
    print("🔧 TESTEUR IRC AMÉLIORÉ")
    print("=" * 60)
    
    # Vérifier le binaire
    if not os.path.exists('./ircserv'):
        print("❌ Erreur: ./ircserv n'existe pas!")
        print("   Compilez d'abord votre projet avec 'make'")
        return 1
    
    # Vérifier le terminal
    terminal_cmd = detect_terminal()
    if not terminal_cmd:
        print("❌ Aucun terminal compatible détecté!")
        print("   Terminaux supportés: gnome-terminal, konsole, xfce4-terminal, xterm, terminator")
        print("\n💡 Vous pouvez utiliser le mode automatique: python3 test_irc.py --auto-kick")
        return 1
    
    print(f"✅ Terminal détecté: {terminal_cmd[0]}")
    
    # Menu interactif
    while True:
        print_menu()
        choice = input("\n👉 Votre choix: ").strip().lower()
        
        if choice == '1':
            scenario_basic(terminal_cmd)
        
        elif choice == '2':
            scenario_kick_tests(terminal_cmd)
        
        elif choice == '3':
            scenario_channel_ops(terminal_cmd)
        
        elif choice == '4':
            launch_server(terminal_cmd)
            print("✅ Serveur lancé!")
        
        elif choice == '5':
            print("\n🤖 Lancement des tests automatiques...")
            print("⚠️  Le serveur doit être lancé!")
            input("Appuyez sur Entrée pour continuer...")
            
            try:
                tester = IRCTester(int(PORT), PASSWORD)
                tester.run_all_tests()
            except Exception as e:
                print(f"❌ Erreur: {e}")
            
            input("\nAppuyez sur Entrée pour continuer...")
        
        elif choice == '6':
            tester = IRCTester(int(PORT), PASSWORD)
            tester.test_kick_basic()
            tester.disconnect_all()
            input("\nAppuyez sur Entrée pour continuer...")
        
        elif choice == '7':
            tester = IRCTester(int(PORT), PASSWORD)
            tester.connect_client("Alice")
            tester.connect_client("Bob")
            tester.test_kick_with_reason()
            tester.disconnect_all()
            input("\nAppuyez sur Entrée pour continuer...")
        
        elif choice == '8':
            tester = IRCTester(int(PORT), PASSWORD)
            tester.test_kick_no_permission()
            tester.disconnect_all()
            input("\nAppuyez sur Entrée pour continuer...")
        
        elif choice == '9':
            show_implementation_checklist()
            input("\nAppuyez sur Entrée pour continuer...")
        
        elif choice == 'c':
            print(f"\nConfiguration actuelle: Port={PORT}, Password={PASSWORD}")
            new_port = input("Nouveau port [6667]: ").strip() or "6667"
            new_pass = input("Nouveau password [42]: ").strip() or "42"
            PORT = new_port
            PASSWORD = new_pass
            SERVER_CMD = f"./ircserv {PORT} {PASSWORD}"
            print(f"✅ Configuration mise à jour!")
        
        elif choice == 'i':
            show_system_info()
            input("\nAppuyez sur Entrée pour continuer...")
        
        elif choice == 'q':
            print("\n👋 Au revoir!")
            break
        
        else:
            print("❌ Choix invalide!")
            time.sleep(1)
    
    return 0


if __name__ == "__main__":
    try:
        sys.exit(main())
    except KeyboardInterrupt:
        print("\n\n⚠️  Interruption utilisateur. Au revoir!")
        sys.exit(0)
