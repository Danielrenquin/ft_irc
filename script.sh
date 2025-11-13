#!/bin/bash

PORT="6667"
PASSWORD="42"
SERVER_CMD="./ircserv $PORT $PASSWORD"

# Commandes pour les clients
CLIENT1_COMMANDS=("PASS 42" "NICK Daniel" "USER Dan 0 * :Dan User")
CLIENT2_COMMANDS=("PASS 42" "NICK Ln" "USER Ln 0 * :Ln User")
CLIENT3_COMMANDS=("PASS 42" "NICK Lilian" "USER Lmonsat 0 * :Lmonsat User")

# -------------------------------------------------------------
# Détection du terminal
# -------------------------------------------------------------
detect_terminal() {
    local terminals=(
        "gnome-terminal --"
        "konsole -e"
        "xfce4-terminal -e"
        "xterm -e"
        "terminator -e"
    )

    for t in "${terminals[@]}"; do
        local bin=$(echo "$t" | awk '{print $1}')
        if command -v "$bin" &>/dev/null; then
            echo "$t"
            return
        fi
    done

    echo ""
}

# -------------------------------------------------------------
# Lancement du serveur
# -------------------------------------------------------------
launch_server() {
    local term=($1)
    echo "🚀 Lancement du serveur IRC..."

    "${term[@]}" bash -c "$SERVER_CMD; read -p 'Appuyez sur Entrée pour fermer...'"
}

# -------------------------------------------------------------
# Lancement d'un client
# -------------------------------------------------------------
launch_client() {
    local term=($1)
    local name="$2"
    shift 2
    local commands=("$@")

    echo "👤 Lancement du client $name..."

    # Construction du script inline
    local script="#!/bin/bash
echo \"=== Connexion en tant que $name ===\"
sleep 1
{
"

    for cmd in "${commands[@]}"; do
        script+="echo \"$cmd\"
sleep 0.5
"
    done

    script+="
cat
} | nc localhost $PORT

read -p \"Appuyez sur Entrée pour fermer...\"
"

    "${term[@]}" bash -c "$script"
}

# -------------------------------------------------------------
# MAIN
# -------------------------------------------------------------
echo "=================================================="
echo "🔧 Script de test IRC automatisé (Bash)"
echo "=================================================="

if [[ ! -f ./ircserv ]]; then
    echo "❌ Erreur : ./ircserv n'existe pas !"
    echo "Compile d'abord avec 'make'."
    exit 1
fi

terminal_cmd=$(detect_terminal)

if [[ -z "$terminal_cmd" ]]; then
    echo "❌ Aucun terminal compatible détecté !"
    echo "Terminaux supportés : gnome-terminal, konsole, xfce4-terminal, xterm, terminator"
    exit 1
fi

echo "✅ Terminal détecté : $terminal_cmd"
echo ""

# Lancer le serveur dans un nouveau terminal
launch_server "$terminal_cmd" &
sleep 2

# Lancer les trois clients
launch_client "$terminal_cmd" "Daniel" "${CLIENT1_COMMANDS[@]}" &
sleep 1
launch_client "$terminal_cmd" "Ln" "${CLIENT2_COMMANDS[@]}" &
sleep 1
launch_client "$terminal_cmd" "Lilian" "${CLIENT3_COMMANDS[@]}" &
sleep 1

echo ""
echo "=================================================="
echo "✅ Tous les terminaux sont lancés !"
echo "=================================================="
echo ""
echo "📝 Instructions :"
echo "   - Le serveur tourne sur le port $PORT"
echo "   - Clients : Daniel / Ln / Lilian"
echo "   - Vous pouvez taper des commandes IRC dans chaque terminal"
echo ""

exit 0
