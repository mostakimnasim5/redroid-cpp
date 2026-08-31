# Firebase Setup for ReDroidCPP

The app talks to **Firebase Firestore over the REST API** using your
project's web API key (no Firebase Auth). Two things must be configured or
you will see:

- **"⚠️ Firebase কনফিগ সঠিক নয়!"** — credentials missing/placeholder, or
- **"Login error (201)"** — HTTP 403 from Firestore because the security
  rules block unauthenticated access.

This guide fixes both for the project **`redroid-d8110`**.

---

## Step 1 — Publish the Firestore security rules

1. Open the Firebase Console: https://console.firebase.google.com/project/redroid-d8110/firestore
2. Go to **Firestore Database → Rules** tab.
3. Replace the contents with the rules from [`firebase/firestore.rules`](../firebase/firestore.rules)
   (copy the whole file).
4. Click **Publish**.

The rules grant exactly what the app needs:

| Collection | Allowed | Why |
|------------|---------|-----|
| `accessRequests` | `create` | Users submit a new access request |
| `activeUsers` | `get`, `list` | App verifies login codes via `:runQuery` |
| `admins` | `get`, `list` | App verifies admin login via `:runQuery` |
| everything else | denied | least privilege |

> ⚠️ **Security warning:** these rules are intentionally open (no
> authentication) — anyone holding the public web API key can read
> `activeUsers`/`admins` and create `accessRequests`. This is acceptable for
> development. For production, add Firebase Authentication to the app and
> tighten the rules (e.g. require `request.auth != null` and scope reads
> per-user). Never store sensitive data in these collections meanwhile.

---

## Step 2 — Give the app your credentials

Three ways, in order of precedence:

### Option A — Settings UI (easiest)

1. Open the app → **Settings → 🔥 Firebase** tab.
2. Enter:
   - **Project ID:** `redroid-d8110`
   - **API Key:** `AIzaSyAItRrMoZyrDtA58aNKt7mTKprBy-4_4gA`
3. Click **Save**. The values are written to
   `%APPDATA%\RedroidCPP\config.json` and take effect immediately.

### Option B — Edit the config file manually

Edit `%APPDATA%\RedroidCPP\config.json`:

```json
{
  "firebase": {
    "projectId": "redroid-d8110",
    "apiKey": "AIzaSyAItRrMoZyrDtA58aNKt7mTKprBy-4_4gA"
  }
}
```

(Keep any other existing keys in the file — only replace the `firebase`
object.)

### Option C — Environment variables (highest precedence)

```powershell
setx REDROID_FB_PROJECT_ID "redroid-d8110"
setx REDROID_FB_API_KEY "AIzaSyAItRrMoZyrDtA58aNKt7mTKprBy-4_4gA"
```

Restart the app afterwards. When these are set, they override the config
file values.

---

## Verify

- Restart the app and try **Login** again — the "Firebase কনফিগ সঠিক নয়!"
  warning should be gone.
- If you still get "Login error (201)", the rules were not published
  correctly (Step 1) — check the Rules tab and the publish timestamp.

## How the app decides

`ConfigManager::hasFirebaseConfig()` returns true only when both values are
present and not placeholders (`YOUR_PROJECT_ID`, `your-firebase-project-id`,
`your-firebase-api-key`, `YOUR_API_KEY`). Environment variables are checked
first, then the config file.
