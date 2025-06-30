import { useEffect, useState } from "react";
import "./App.css";
import useWebSocket from "react-use-websocket";

type MessageBody = {
  action: string
  type: string
  body: unknown
}

type SecurityStatus = {
  state: "disarmed" | "arming" | "armed" | "intrusion" | "alarm"
  led: boolean
  alarm: boolean
}

const outputPins = [13, 12]
const defaultOutputPin = outputPins[0];

function App() {
  const { lastMessage, sendMessage, readyState } = useWebSocket(
    "wss://aneblnoqd6.execute-api.us-east-1.amazonaws.com/dev",
  );

  const [selectedPin, setSelectedPin] = useState(defaultOutputPin);
  const [pinValue, setPinValue] = useState(false);
  const [securityStatus, setSecurityStatus] = useState<SecurityStatus>({
    state: "disarmed",
    led: false,
    alarm: false
  });
  const [connectionStatus, setConnectionStatus] = useState("Connecting...");

  useEffect(() => {
    if (lastMessage === null) {
      return;
    }

    const parsedMessage = JSON.parse(lastMessage.data) as MessageBody;

    if (parsedMessage.action !== "msg") {
      return;
    }

    if (parsedMessage.type === "output") {
      const body = parsedMessage.body as number;
      setPinValue(body === 0 ? false : true);
    } else if (parsedMessage.type === "security_status") {
      const body = parsedMessage.body as SecurityStatus;
      setSecurityStatus(body);
    } else if (parsedMessage.type === "error") {
      console.error("Erreur ESP32:", parsedMessage.body);
      alert(`Erreur: ${parsedMessage.body}`);
    } else if (parsedMessage.type === "status" && parsedMessage.body === "ok") {
      console.log("Commande exécutée avec succès");
    }
  }, [lastMessage]);

  useEffect(() => {
    switch (readyState) {
      case 0: // CONNECTING
        setConnectionStatus("Connexion...");
        break;
      case 1: // OPEN
        setConnectionStatus("Connecté");
        // Petite attente pour que la connexion soit stable
        setTimeout(() => {
          // Demander le statut de sécurité au démarrage
          sendMessage(JSON.stringify({
            action: "msg",
            type: "cmd",
            body: {
              type: "security",
              action: "status"
            }
          }));
          console.log("Demande de statut envoyée");
        }, 1000);
        break;
      case 2: // CLOSING
        setConnectionStatus("Fermeture...");
        break;
      case 3: // CLOSED
        setConnectionStatus("Déconnecté");
        break;
    }
  }, [readyState, sendMessage]);

  useEffect(() => {
    sendMessage(JSON.stringify({
      action: "msg",
      type: "cmd",
      body: {
        type: "digitalRead",
        pin: defaultOutputPin,
      }
    }))
    outputPins.forEach((pin) => {
      sendMessage(JSON.stringify({ 
        action: "msg",
        type: "cmd",
        body: {
          type: "pinMode",
          pin,
          mode: "output",
        }
      }))
    })
  }, [sendMessage])

  const handleSecurityAction = (action: "arm" | "disarm") => {
    sendMessage(JSON.stringify({
      action: "msg",
      type: "cmd",
      body: {
        type: "security",
        action: action
      }
    }));
  };

  const getSecurityStateDisplay = (state: string) => {
    switch (state) {
      case "disarmed": return { text: "Désarmé", color: "text-green-600", bg: "bg-green-100" };
      case "arming": return { text: "Armement...", color: "text-yellow-600", bg: "bg-yellow-100" };
      case "armed": return { text: "Armé", color: "text-blue-600", bg: "bg-blue-100" };
      case "intrusion": return { text: "Intrusion!", color: "text-orange-600", bg: "bg-orange-100" };
      case "alarm": return { text: "ALARME!", color: "text-red-600", bg: "bg-red-100" };
      default: return { text: "Inconnu", color: "text-gray-600", bg: "bg-gray-100" };
    }
  };

  const stateDisplay = getSecurityStateDisplay(securityStatus.state);

  return (
    <div className="App min-h-screen bg-gray-50 p-4">
      <div className="max-w-4xl mx-auto">
        <h1 className="text-3xl font-bold text-center mb-8 text-gray-800">
          Système de Sécurité ESP32
        </h1>
        
        {/* Statut de connexion */}
        <div className="mb-6 text-center">
          <span className={`inline-block px-3 py-1 rounded-full text-sm font-medium ${
            readyState === 1 ? 'bg-green-100 text-green-800' : 'bg-red-100 text-red-800'
          }`}>
            {connectionStatus}
          </span>
        </div>

        <div className="grid grid-cols-1 lg:grid-cols-2 gap-6">
          {/* Panneau de Sécurité */}
          <div className="bg-white rounded-lg shadow-lg p-6">
            <h2 className="text-xl font-semibold mb-4 text-gray-800">Système de Sécurité</h2>
            
            {/* Statut actuel */}
            <div className="mb-6">
              <div className="text-sm text-gray-600 mb-2">Statut actuel:</div>
              <div className={`inline-block px-4 py-2 rounded-lg font-semibold text-lg ${stateDisplay.bg} ${stateDisplay.color}`}>
                {stateDisplay.text}
              </div>
            </div>

            {/* Indicateurs */}
            <div className="grid grid-cols-2 gap-4 mb-6">
              <div className="flex items-center space-x-2">
                <div className={`w-3 h-3 rounded-full ${securityStatus.led ? 'bg-yellow-400' : 'bg-gray-300'}`}></div>
                <span className="text-sm text-gray-600">LED</span>
              </div>
              <div className="flex items-center space-x-2">
                <div className={`w-3 h-3 rounded-full ${securityStatus.alarm ? 'bg-red-500 animate-pulse' : 'bg-gray-300'}`}></div>
                <span className="text-sm text-gray-600">Alarme</span>
              </div>
            </div>

            {/* Contrôles de sécurité */}
            <div className="space-y-3">
              <button
                onClick={() => handleSecurityAction("arm")}
                disabled={securityStatus.state !== "disarmed" || readyState !== 1}
                className="w-full bg-blue-600 hover:bg-blue-700 disabled:bg-gray-400 text-white font-medium py-3 px-4 rounded-lg transition-colors"
              >
                Armer le Système
              </button>
              
              <button
                onClick={() => handleSecurityAction("disarm")}
                disabled={securityStatus.state === "disarmed" || readyState !== 1}
                className="w-full bg-red-600 hover:bg-red-700 disabled:bg-gray-400 text-white font-medium py-3 px-4 rounded-lg transition-colors"
              >
                Désarmer le Système
              </button>
            </div>

            {/* Aide */}
            <div className="mt-6 p-4 bg-gray-50 rounded-lg">
              <h3 className="text-sm font-medium text-gray-800 mb-2">Contrôles Physiques:</h3>
              <ul className="text-xs text-gray-600 space-y-1">
                <li>• Appui court: ON/OFF LED</li>
                <li>• Appui long (5s): Armer/Désarmer</li>
              </ul>
            </div>
          </div>

          {/* Panneau de Contrôle GPIO */}
          <div className="bg-white rounded-lg shadow-lg p-6">
            <h2 className="text-xl font-semibold mb-4 text-gray-800">Contrôle des lampes</h2>
            
            <div className="space-y-4">
              <form className="max-w-sm">
                <label className="block mb-2 text-base font-medium text-gray-900">
                  Sélectionner une lampe
                </label>
                <select
                  value={selectedPin}
                  onChange={(e) => {
                    const newPin = parseInt(e.target.value, 10)
                    setSelectedPin(newPin)
                    sendMessage(JSON.stringify({
                      action: "msg",
                      type: "cmd",
                      body: {
                        type: "digitalRead",
                        pin: newPin,
                      }
                    }))
                  }}
                  className="bg-gray-50 border border-gray-300 text-gray-900 text-sm rounded-lg focus:ring-blue-500 focus:border-blue-500 block w-full p-2.5"
                >
                  {outputPins.map((pin) => (
                    <option key={pin} value={pin}>Lampe {pin}</option>
                  ))}
                </select>
              </form>

              <div className="mt-4">
                <label className="inline-flex items-center cursor-pointer">
                  <input
                    type="checkbox"
                    checked={pinValue}
                    onChange={() => {
                      const newValue = !pinValue;
                      setPinValue(newValue);
                      sendMessage(JSON.stringify({ 
                        action: "msg",
                        type: "cmd",
                        body: {
                          type: "digitalWrite",
                          pin: selectedPin,
                          value: newValue ? 1 : 0,
                        }
                      }))
                    }}
                    className="sr-only peer"
                    disabled={readyState !== 1}
                  />
                  <div className="relative w-14 h-7 bg-gray-200 peer-focus:outline-none peer-focus:ring-4 peer-focus:ring-blue-300 rounded-full peer peer-checked:after:translate-x-full peer-checked:after:border-white after:content-[''] after:absolute after:top-0.5 after:start-[4px] after:bg-white after:border-gray-300 after:border after:rounded-full after:h-6 after:w-6 after:transition-all peer-checked:bg-blue-600"></div>
                  <span className="ms-3 text-sm font-medium text-gray-900">
                    {pinValue ? ". . . . . Activé" : ". . . . . Désactivé"}
                  </span>
                </label>
              </div>
            </div>

            {/* Informations système */}
            <div className="mt-6 p-4 bg-gray-50 rounded-lg">
              <h3 className="text-sm font-medium text-gray-800 mb-2">Informations Système:</h3>
              <div className="text-xs text-gray-600 space-y-1">
                <div>Pin sélectionné: GPIO{selectedPin}</div>
                <div>État: {pinValue ? "HIGH" : "LOW"}</div>
                <div>WebSocket: {connectionStatus}</div>
              </div>
            </div>
          </div>
        </div>

        {/* Instructions d'utilisation */}
        <div className="mt-8 bg-white rounded-lg shadow-lg p-6">
          <h2 className="text-xl font-semibold mb-4 text-gray-800">Instructions d'Utilisation</h2>
          <div className="grid grid-cols-1 md:grid-cols-2 gap-6">
            <div>
              <h3 className="font-medium text-gray-800 mb-2">Système de Sécurité:</h3>
              <ul className="text-sm text-gray-600 space-y-1">
                <li>• <strong>Désarmé:</strong> Système inactif, LED contrôlable manuellement</li>
                <li>• <strong>Armement:</strong> 20 secondes pour quitter les lieux</li>
                <li>• <strong>Armé:</strong> Surveillance active des capteurs</li>
                <li>• <strong>Intrusion:</strong> 30 secondes pour désarmer</li>
                <li>• <strong>Alarme:</strong> Alarme sonore active</li>
              </ul>
            </div>
            <div>
              <h3 className="font-medium text-gray-800 mb-2">Capteurs Surveillés:</h3>
              <ul className="text-sm text-gray-600 space-y-1">
                <li>• <strong>Capteur sonore:</strong> Détection de bruit (Pin A2)</li>
                <li>• <strong>Capteur ultrasonique:</strong> Détection de mouvement (Pin A3)</li>
                <li>• <strong>Capteur tactile:</strong> Contrôle physique (Pin D14)</li>
                <li>• <strong>Speaker:</strong> Signaux sonores (Pin D21)</li>
              </ul>
            </div>
          </div>
        </div>
      </div>
    </div>
  );
}

export default App;