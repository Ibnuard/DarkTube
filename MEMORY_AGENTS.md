# DarkTube – Networking & Memory Safety Patch

## Context

Atmosphere crash occurs when fetching trending videos from Invidious.
Likely cause: large JSON response causing memory exhaustion or heap fragmentation.

Nintendo Switch homebrew has limited RAM and small stack.
We must aggressively limit memory usage.

---

# Goals

1. Prevent Atmosphere crash
2. Limit JSON response size
3. Parse only MAX_VIDEOS entries
4. Free all allocated memory properly
5. Avoid large stack allocations

---

# Required Changes

## 1️⃣ Limit Download Size in http_get()

In `network.c`, inside http_get():

Add:

```c
curl_easy_setopt(curl_handle, CURLOPT_MAXFILESIZE_LARGE, (curl_off_t)500000);
```

Max 500KB response.

After request completes, add safety check:

```c
if (chunk.size > 500000) {
    printf("Response too large, aborting.\n");
    curl_easy_cleanup(curl_handle);
    free(chunk.memory);
    return -1;
}
```

---

## 2️⃣ Add Response Size Debug Log

After successful curl request:

```c
printf("HTTP response size: %zu bytes\n", chunk.size);
```

This helps detect oversized JSON.

---

## 3️⃣ Limit JSON Parsing to MAX_VIDEOS

Inside fetch_trending():

When iterating JSON array:

Replace:

```c
for (int i = 0; i < array_size; i++)
```

With:

```c
for (int i = 0; i < array_size && count < MAX_VIDEOS; i++)
```

Do NOT parse entire array.

---

## 4️⃣ Ensure JSON Cleanup

After parsing:

```c
cJSON_Delete(root);
free(response);
```

Memory must always be freed.

---

## 5️⃣ Avoid Large Stack Allocation

If VideoItem contains large fields:

❌ BAD:
```c
VideoItem items[MAX_VIDEOS];
```

✅ GOOD:
```c
VideoItem *items = malloc(sizeof(VideoItem) * MAX_VIDEOS);
```

Free before exit:
```c
free(items);
```

---

## 6️⃣ Optional: Reduce API Payload

Use lighter endpoint:

```
/api/v1/trending?type=music
```

Or switch to search endpoint with limited results.

---

# Validation Checklist

- [ ] socketInitializeDefault() called only once in main
- [ ] curl_global_init() called in init_network()
- [ ] Response size printed
- [ ] JSON limited to MAX_VIDEOS
- [ ] cJSON_Delete() called
- [ ] No stack overflow
- [ ] No Atmosphere crash

---

# Target Behavior

App must:

- Fetch trending videos
- Display up to MAX_VIDEOS entries
- Never crash due to large JSON
- Fail gracefully if response > 500KB

---

# Priority

HIGH – Stability before feature expansion.

Do NOT implement streaming or caching until memory stability is confirmed.