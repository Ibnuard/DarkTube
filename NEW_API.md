# DarkTube API Documentation

Dokumentasi lengkap untuk endpoint API DarkTube Server.

## Base URL
`http://localhost:3000` (atau IP VPS Anda)

---

## 0. Server Status / Health Check
Mengecek status server, versi, cookies, PO Token provider, dan yt-dlp.

- **URL**: `/`
- **Method**: `GET`
- **Response Example**:
  ```json
  {
    "name": "DarkTube Server",
    "version": "1.3.0",
    "status": "online",
    "health": "ok",
    "environment": "production",
    "region": "ID",
    "ytdlpVersion": "2026.02.21",
    "cookies": {
      "loaded": true,
      "valid": true,
      "entries": 22,
      "size": 2988,
      "reason": "OK"
    },
    "potProvider": {
      "available": true,
      "url": "http://pot-provider:4416"
    }
  }
  ```

---

## 1. Search Videos
Mencari video berdasarkan kata kunci.

- **URL**: `/api/search`
- **Method**: `GET`
- **Query Parameters**:
  - `q` (string, required): Kata kunci pencarian.
  - `maxResults` (number, optional): Jumlah hasil (default: 10).
  - `pageToken` (string, optional): Token untuk halaman berikutnya.

- **Response Example**:
  ```json
  {
    "videos": [
      {
        "id": "dQw4w9WgXcQ",
        "title": "Rick Astley - Never Gonna Give You Up",
        "description": "...",
        "thumbnails": { "default": { "url": "...", "width": 120, "height": 90 } },
        "channelTitle": "Rick Astley",
        "publishedAt": "2009-10-25T06:57:33Z"
      }
    ],
    "nextPageToken": "CAoQAA"
  }
  ```

---

## 2. Trending Videos
Mendapatkan video populer berdasarkan wilayah.

- **URL**: `/api/trending`
- **Method**: `GET`
- **Query Parameters**:
  - `regionCode` (string, optional): Kode negara (ISO 3166-1 alpha-2), contoh: `ID`, `US`, `JP`. Menggunakan default dari `.env` jika tidak diisi.
  - `maxResults` (number, optional): Jumlah hasil (default: 10).
  - `pageToken` (string, optional): Token untuk halaman berikutnya.

- **Response Example**:
  ```json
  {
    "videos": [
      {
        "id": "...",
        "title": "...",
        "statistics": {
          "viewCount": "1000000",
          "likeCount": "50000",
          "commentCount": "2000"
        }
      }
    ]
  }
  ```

---

## 3. Video Details
Mendapatkan detail lengkap dan statistik suatu video.

- **URL**: `/api/video/:id`
- **Method**: `GET`

- **Response Example**: Meta data lengkap dari YouTube API v3 (Snippet, Statistics, ContentDetails).

---

## 4. Get Stream URL
Mengekstrak direct URL video untuk diputar di player media. Menggunakan PO Token dan cookies untuk bypass bot detection.

- **URL**: `/api/stream`
- **Method**: `GET`
- **Query Parameters**:
  - `id` (string, required): ID video YouTube.

- **Response Example (Success)**:
  ```json
  {
    "title": "Video Title",
    "url": "https://rr5---sn-...",
    "thumbnail": "...",
    "duration": 212,
    "strategy": "pot+cookies",
    "formats": [
      {
        "format_id": "18",
        "ext": "mp4",
        "resolution": "640x360",
        "url": "...",
        "filesize": 12345678,
        "vcodec": "avc1.42001E",
        "acodec": "mp4a.40.2"
      }
    ]
  }
  ```

- **Response Example (Error)**:
  ```json
  {
    "error": "All extraction strategies failed",
    "lastError": "ERROR: ...",
    "diagnostics": {
      "ytdlpVersion": "2026.02.21",
      "cookies": { "loaded": true, "valid": true },
      "potProvider": { "available": true },
      "hints": ["..."]
    }
  }
  ```

---

## 5. Update yt-dlp (Admin)
Update yt-dlp dan PO Token plugin ke versi terbaru tanpa rebuild Docker.

- **URL**: `/api/admin/update-ytdlp`
- **Method**: `POST`

- **Response Example**:
  ```json
  {
    "success": true,
    "versionBefore": "2026.02.21",
    "versionAfter": "2026.02.25",
    "updated": true,
    "output": "Successfully installed yt-dlp-2026.02.25"
  }
  ```

---

## 6. Cookies Status (Admin)
Mengecek detail status cookies yang dimuat.

- **URL**: `/api/admin/cookies`
- **Method**: `GET`

- **Response Example**:
  ```json
  {
    "loaded": true,
    "valid": true,
    "entries": 22,
    "size": 2988,
    "reason": "OK"
  }
  ```

---

## Error Codes
- `400 Bad Request`: Parameter kurang atau salah.
- `404 Not Found`: Video tidak ditemukan.
- `500 Internal Server Error`: Masalah pada server atau YouTube API.

---

## Architecture
Server menggunakan arsitektur multi-container:

| Container | Fungsi |
|---|---|
| `api` | Node.js Express server, menjalankan yt-dlp |
| `pot-provider` | Generate PO Token (Proof of Origin) untuk bypass bot detection YouTube |

### Anti-Bot Strategy
1. **PO Token** — Plugin `bgutil-ytdlp-pot-provider` generate token otomatis via sidecar container
2. **Cookies** — YouTube account cookies untuk autentikasi
3. **EJS** — JavaScript challenge solver untuk YouTube signature decryption
4. **Auto-update** — yt-dlp dan plugin diupdate otomatis setiap container restart
