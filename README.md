# 🏠 MyHouseOS Server - Serveur d'Authentification Domotique

## 📋 Description

Serveur d'authentification et de gestion centralisé pour dispositifs ESP32 utilisant le M5Stack CoreS3. Ce projet transforme votre M5Stack en un **hub WiFi autonome** qui gère l'appairage sécurisé et la supervision des dispositifs IoT domotiques.

Ce projet implémente un **serveur ESP32** qui agit comme point d'accès WiFi et contrôleur central pour :

- Créer un réseau WiFi autonome pour dispositifs domotiques
- Gérer l'authentification sécurisée des dispositifs via tokens
- Superviser les connexions/déconnexions en temps réel
- Valider manuellement les demandes d'appairage
- Fournir une interface graphique de monitoring

## ✨ Fonctionnalités

### 🖥️ Interface Graphique

- **Interface moderne** avec design card-based et palette de couleurs professionnelle
- **Affichage temps réel** des informations système :
  - 📡 Logo MyHouseOS avec IP du serveur
  - 👥 Nombre de clients connectés en direct
  - 📝 Logs d'activité avec historique de 10 événements
- **Codage couleur** des événements (vert=connexion, orange=déconnexion, bleu=info)
- **Écran de confirmation** pour les demandes de pairing

### 🔐 Gestion d'Authentification

- Point d'accès WiFi autonome (AP Mode)
- Endpoint `/link` pour appairage de nouveaux dispositifs
- Génération automatique de tokens UUID uniques
- Vérification via API externe si le dispositif existe déjà
- Communication sécurisée avec l'API backend (Authorization header)
- Réponse JSON structurée avec statut et token

### 📡 Supervision Réseau

- **Monitoring automatique** toutes les 2 secondes
- Détection des connexions/déconnexions de clients
- Affichage des adresses MAC et IP DHCP
- Gestion jusqu'à 10 clients simultanés
- Logs détaillés dans le moniteur série

### 🎮 Contrôle Physique

- **Bouton A (BtnA)** : Accepter une demande d'appairage
- **Bouton B (BtnB)** : Rejeter une demande d'appairage
- Timeout de 1 seconde pour les validations manuelles
- Feedback visuel immédiat des actions utilisateur

## 🔧 Configuration Matérielle

### Matériel Requis

- **M5Stack CoreS3** (ESP32-S3)
- **Câble USB-C** pour programmation et alimentation
- **Réseau WiFi** (optionnel pour communication avec API externe)

### Aucun câblage externe requis

Le serveur fonctionne de manière autonome sans composants externes.

## ⚙️ Configuration Logicielle

### Bibliothèques Nécessaires

```cpp
#include "M5CoreS3.h"        // Librairie M5Stack CoreS3
#include <WiFi.h>            // Gestion WiFi
#include <esp_wifi.h>        // API WiFi ESP32
#include <esp_netif.h>       // Interface réseau
#include <ESPAsyncWebServer.h> // Serveur Web asynchrone
#include <HTTPClient.h>      // Client HTTP pour API externe
#include <ArduinoJson.h>     // Parsing JSON
```

### Paramètres Réseau

```cpp
// Point d'accès WiFi créé par le serveur
SSID: "MyHouseOS"
Password: "12345678"
IP: 192.168.4.1
Gateway: 192.168.4.1
Subnet: 255.255.255.0

// API externe pour validation (optionnel)
const char* externalAPICheckURL = "http://192.168.4.2:3000/check";
const char* externalAPIAuthURL = "http://192.168.4.2:3000/auth";

// Authentification API
Authorization: master:master
```

### Limites Configurables

```cpp
#define MAX_LOGS 10                        // Nombre de logs affichés
#define MAX_CLIENTS 10                     // Clients WiFi maximum
const unsigned long UPDATE_INTERVAL = 2000; // Rafraîchissement (ms)
const unsigned long TIMEOUT = 1000;        // Timeout pairing (ms)
```

## 🚀 Installation

### 1. Prérequis

- [Arduino IDE](https://www.arduino.cc/en/software) ou PlatformIO
- [M5Stack Library](https://github.com/m5stack/M5CoreS3)
- [ESPAsyncWebServer](https://github.com/me-no-dev/ESPAsyncWebServer)
- [ArduinoJson](https://arduinojson.org/) (v7+)

### 2. Installation des Bibliothèques

Dans Arduino IDE :

```
Outils → Gérer les bibliothèques
Rechercher et installer :
- M5CoreS3
- ESPAsyncWebServer (me-no-dev)
- ArduinoJson
```

### 3. Configuration

1. Ouvrir `Server.ino`
2. Modifier le SSID/password WiFi si désiré
3. Ajuster les URLs de l'API externe selon votre backend
4. Personnaliser les couleurs si souhaité

### 4. Upload

1. Connecter le M5Stack CoreS3 via USB-C
2. Sélectionner le port COM approprié
3. Board: "M5Stack-CoreS3"
4. Téléverser le code

## 📱 Utilisation

### Au Démarrage

1. Le M5Stack affiche "STARTING..."
2. Le point d'accès WiFi "MyHouseOS" est créé
3. L'interface principale s'affiche avec :
   - Logo et IP du serveur (192.168.4.1)
   - Compteur de clients (initialement 0)
   - Zone de logs d'activité

### Processus d'Appairage

#### Côté Client (Dispositif ESP32)

1. Le dispositif se connecte au WiFi "MyHouseOS"
2. Il envoie une requête POST à `http://192.168.4.1/link`
3. Corps de la requête : `{"id":"ESP32_DEVICE_001"}`

#### Côté Serveur (M5Stack)

1. Réception de la demande → Log "Link request: [ID]"
2. **Écran de confirmation** s'affiche :
   ```
   PAIRING REQUEST
   ESP32_DEVICE_001
   
   BtnA = Accept
   BtnB = Reject
   ```
3. L'utilisateur a 1 seconde pour décider
4. **Si accepté (BtnA)** :
   - Vérification si le dispositif existe déjà (via API externe)
   - Si oui : récupération du token existant
   - Si non : génération d'un nouveau token UUID
   - Enregistrement dans l'API externe
   - Réponse au client avec le token
5. **Si rejeté (BtnB)** ou timeout :
   - Réponse d'erreur au client
   - Retour à l'interface principale

### Suivi des Clients

L'écran affiche en temps réel :
- **Connexions** : "Connected: 192.168.4.X" (vert)
- **Déconnexions** : "Disconnected: 192.168.4.X" (orange)
- **Événements système** : Logs avec timestamps implicites

## 🌐 API et Communication

### Endpoints Serveur

#### POST `/link` - Appairage de dispositif

**Request:**
```json
{
  "id": "F8C096E350CC"
}
```

**Response (succès):**
```json
{
  "status": "success",
  "token": "a1b2c3d4-e5f6-7890-abcd-ef1234567890"
}
```

**Response (erreurs possibles):**
```json
// JSON invalide
{
  "status": "error",
  "message": "Invalid JSON"
}

// ID manquant
{
  "status": "error",
  "message": "Missing or empty ID"
}

// Rejeté par utilisateur
{
  "status": "error",
  "message": "Connection rejected by user"
}

// Timeout
{
  "status": "error",
  "message": "Request timeout"
}

// API externe inaccessible
{
  "status": "error",
  "message": "Check API unreachable"
}
```

#### GET `/status` - État du serveur

**Response:**
```json
{
  "status": "running"
}
```

### Communication avec API Externe

#### GET `/check?id=[DEVICE_ID]`

Vérifie si un dispositif existe déjà.

**Headers:**
```
Authorization: master:master
```

**Response:**
```json
{
  "exists": true,
  "token": "existing-token-here"
}
```

#### POST `/auth`

Enregistre un nouveau dispositif.

**Headers:**
```
Authorization: master:master
Content-Type: application/json
```

**Body:**
```json
{
  "id": "F8C096E350CC",
  "token": "a1b2c3d4-e5f6-7890-abcd-ef1234567890"
}
```

## 🎨 Interface Utilisateur

### Palette de Couleurs

```cpp
Fond principal : #0f172a (Bleu nuit slate-900)
Cartes         : #1e293b (Gris ardoise slate-800)
Accent         : #6366f1 (Indigo-500)
Texte          : #f8fafc (Blanc cassé slate-50)
Sous-texte     : #94a3b8 (Gris clair slate-400)
Succès         : #22c55e (Vert green-500)
Avertissement  : #fb923c (Orange orange-400)
```

### Structure de l'Écran Principal

```
┌─────────────────────────────────┐
│ MyHouseOS  │    CLIENTS         │
│ 192.168.4.1│       3            │ Header (cards)
├─────────────────────────────────┤
│ ACTIVITY LOGS                   │
│                                 │
│ ● Connected: 192.168.4.5        │
│ ● Link request: ESP32_001       │
│ ● Accepted: ESP32_001           │ Logs (8 derniers)
│ ● API OK: ESP32_001             │
│ ● New device: ESP32_001         │
│ ● System started                │
│ ● Server running                │
└─────────────────────────────────┘
```

### Écran de Pairing

```
┌─────────────────────────────────┐
│                                 │
│      PAIRING REQUEST            │
│                                 │
│      F8C096E350CC               │
│                                 │
│                                 │
│      BtnA = Accept              │
│      BtnB = Reject              │
│                                 │
└─────────────────────────────────┘
```

## 🔄 Architecture et Flux de Données

```
┌─────────────────┐     WiFi      ┌──────────────┐
│  Dispositif     │ ────────────→ │  M5Stack     │
│  ESP32 Client   │   MyHouseOS   │  Serveur AP  │
└─────────────────┘               └──────────────┘
                                         │
                    HTTP POST /link      │
                    {"id":"..."}         │
                                         ↓
                              ┌──────────────────┐
                              │ Validation User  │
                              │  BtnA / BtnB     │
                              └──────────────────┘
                                         │
                         Si accepté      │
                                         ↓
                              ┌──────────────────┐     HTTP
                              │  API Externe     │ ←─────────
                              │  192.168.4.2     │
                              │  - /check        │
                              │  - /auth         │
                              └──────────────────┘
                                         │
                         Token généré    │
                                         ↓
                              ┌──────────────────┐
                              │  Response JSON   │
                              │  avec token      │
                              └──────────────────┘
```

### Cycle de Monitoring (2s)

1. **updateClientList()** → Scan stations WiFi connectées
2. Récupération des IPs via DHCP
3. Comparaison avec liste précédente
4. Détection connexions/déconnexions
5. Mise à jour logs et compteur
6. **drawInterface()** → Rafraîchissement écran

## 🐛 Dépannage

### Problème : Le point d'accès ne se crée pas

- Vérifier que le M5Stack démarre correctement
- Ouvrir le moniteur série (115200 bauds)
- Chercher "MyHouseOS started" dans les logs
- Vérifier qu'aucun autre programme n'utilise le WiFi

### Problème : Les clients ne peuvent pas se connecter

- Vérifier le SSID : "MyHouseOS"
- Vérifier le mot de passe : "12345678"
- S'assurer que MAX_CLIENTS (10) n'est pas atteint
- Redémarrer le M5Stack

### Problème : Erreurs API externe

- Vérifier que l'API backend est accessible
- Ping 192.168.4.2 depuis un client connecté
- Vérifier les URLs dans le code
- Vérifier le header Authorization: "master:master"
- Consulter les logs série pour détails HTTP

### Problème : Timeout lors du pairing

- Par défaut, timeout = 1 seconde
- Modifier `TIMEOUT` pour plus de temps
- Vérifier que les boutons physiques fonctionnent
- Le M5Stack doit recevoir M5.update() dans la boucle

### Problème : Logs ne s'affichent pas

- Vérifier que MAX_LOGS = 10
- Les logs sont circulaires (les anciens sont écrasés)
- L'écran se rafraîchit toutes les 2 secondes
- Vérifier l'appel à drawInterface()

### Problème : Écran noir ou figé

- Appuyer sur le bouton Reset
- Vérifier l'alimentation USB-C
- Vérifier dans le moniteur série si le code tourne
- Tester avec un code minimal M5CoreS3

## 📝 Personnalisation

### Changer le SSID et Mot de Passe

```cpp
WiFi.softAP("MonReseauPerso", "MotDePasseSecurise");
```

### Modifier l'Intervalle de Rafraîchissement

```cpp
const unsigned long UPDATE_INTERVAL = 5000; // 5 secondes
```

### Augmenter le Nombre de Clients

```cpp
#define MAX_CLIENTS 20 // 20 clients max
```

### Changer le Timeout de Pairing

```cpp
const unsigned long TIMEOUT = 60000; // 60 secondes
```

### Personnaliser les Couleurs

```cpp
#define COLOR_BG CoreS3.Display.color565(0, 0, 0) // Fond noir
#define COLOR_ACCENT CoreS3.Display.color565(255, 0, 0) // Accent rouge
```

### Ajouter des Logs Personnalisés

```cpp
addLog("Mon message", COLOR_SUCCESS); // Vert
addLog("Attention", COLOR_WARNING);   // Orange
addLog("Information", COLOR_ACCENT);  // Bleu
```

## 🔒 Sécurité

⚠️ **Important** : Ce code est conçu pour un environnement de développement/test local.

### Recommandations pour Production

- **Changer le mot de passe WiFi** : Ne pas utiliser "12345678"
- **Sécuriser l'API externe** : Utiliser des tokens robustes, pas "master:master"
- **Implémenter HTTPS** : Pour les communications sensibles
- **Validation côté serveur** : Vérifier tous les inputs JSON
- **Rate limiting** : Limiter les tentatives de pairing
- **Logs sécurisés** : Ne pas afficher de données sensibles
- **Timeout adapté** : 1s peut être court en production
- **Whitelist MAC** : Optionnellement filtrer par adresse MAC

## 📄 Licence

Projet open source - Libre d'utilisation et de modification.

## 👥 Auteur

Développé pour le M5Stack CoreS3 dans le cadre d'un projet de domotique connectée MyHouseOS.

---

**Version :** 1.0  
**Date :** Décembre 2025  
**Plateforme :** M5Stack CoreS3 (ESP32-S3)
