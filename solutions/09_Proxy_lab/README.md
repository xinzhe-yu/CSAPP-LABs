# Concurrent Caching Web Proxy

An HTTP/1.0 forward proxy in C. Thread-per-connection, with a shared LRU object cache
guarded by a readers-writers lock. Scores 70/70 on the CS:APP proxy driver.

Source: [`proxy.c`](../../Labs/proxylab-handout/proxylab-handout/proxy.c) ·
[`cache.c`](../../Labs/proxylab-handout/proxylab-handout/cache.c) ·
[`cache.h`](../../Labs/proxylab-handout/proxylab-handout/cache.h)

## Results

| section | what it checks | score |
|---|---|---|
| basic | 5 files fetched through the proxy vs. directly, compared byte-for-byte | 40/40 |
| concurrency | a second request served while one is blocked on an unresponsive origin | 15/15 |
| cache | object still served after the origin server is killed | 15/15 |
| **total** | | **70/70** |

The basic set covers both text and binary: `home.html`, `csapp.c`, `tiny.c`,
`godzilla.jpg`, and the `tiny` ELF binary itself.

Progression, by milestone commit:

| version | capability |
|---|---|
| `Single thread` | listening socket, accept loop |
| `proxy client done` | forwards to origin, relays response |
| `URI` | absolute-URI parsing, host/port/path split |
| `basic done` | header rewriting; 40/70 |
| `multithread` | thread per connection; 55/70 |
| `Finished Proxy` | LRU cache with readers-writers locking; 70/70 |

## Request flow

```
   client                     proxy                        origin
     |                          |                             |
     |---- GET http://h/p ----->|                             |
     |                          | parse_uri -> host,port,path |
     |                          | cache_find(request)         |
     |                          |                             |
     |                          |--- hit? ---> write cached ->|  (no origin contact)
     |                          |                             |
     |                          |--- miss ---- GET /p ------->|
     |<--- relay response ------|<---------- response --------|
     |                          | cache_insert if <= 100 KB   |
```

Each accepted connection is handed to a detached thread. `main` mallocs the descriptor
per connection rather than passing `&connfd` — the loop would otherwise overwrite it
before the new thread dereferences it.

## Header rewriting

The request line is rewritten to HTTP/1.0 and the path made origin-relative. Client
headers pass through except:

| header | action |
|---|---|
| `Host:` | forwarded as sent; synthesized from the URI if absent |
| `User-Agent:` | replaced with the fixed course string |
| `Connection:` | replaced with `close` |
| `Proxy-Connection:` | replaced with `close` |
| everything else | forwarded unchanged |

## URI parsing

`parse_uri` splits an absolute URI into host, port, and path, defaulting the port to 80.
The parse is positional rather than tokenized, because a colon means "port" only when it
precedes the first slash:

| URI | host | port | path |
|---|---|---|---|
| `http://host:8080/a.html` | `host` | 8080 | `/a.html` |
| `http://host/a:b/c` | `host` | 80 | `/a:b/c` |
| `http://host/x?t=1:2` | `host` | 80 | `/x?t=1:2` |
| `http://host` | `host` | 80 | `/` |
| `host/index.html` | `host` | 80 | `/index.html` |

A colon after the first slash is path data, not a port separator — `host/a:b/c` is the
case that catches a naive `strchr(uri, ':')`.

## Cache

Ten slots, each holding an object up to `MAX_OBJECT_SIZE` (100 KB), for 1,024,000 bytes
worst case — inside the 1,049,000-byte budget by construction, so no running total is
needed. Objects larger than 100 KB stream through uncached.

Eviction is LRU by a monotonic counter: every hit stamps the block with the current
`tick`, and `cache_evict` frees the lowest. A response is cached only if the relay loop
ended cleanly (`n == 0`) and the object is non-empty — otherwise a connection that failed
mid-transfer would poison the cache with a truncated object served to every later client.

### Synchronization

Readers-writers with a reader-preference protocol. `cache_find` is a reader; `cache_insert`
(and the `cache_evict` it calls) is the writer:

```
reader entry            reader exit             writer
  P(mutex)                P(mutex)                P(w)
  readcnt++               readcnt--                 ... mutate ...
  if 1: P(w)              if 0: V(w)              V(w)
  V(mutex)                V(mutex)
```

`w` is held by the *group* of readers: the first in acquires it, the last out releases it,
so a writer cannot evict a block while any reader is inside `memcpy`. `mutex` protects
`readcnt` itself.

The LRU stamp is the subtlety. `tick++` and `cache[i].time = tick` are writes performed
on the *read* path, so concurrent readers race on them even though the protocol is
otherwise sound. They are guarded separately:

```c
P(&mutex);
tick++;
cache[i].time = tick;
V(&mutex);
```

Reusing `mutex` rather than adding a fourth semaphore is safe here because the LRU path
never acquires `w` while holding `mutex`, so no cycle exists.

No network I/O happens under either lock — `cache_find` runs before connecting and
`cache_insert` after the transfer completes — so a slow origin never serializes the proxy.
That is what keeps the concurrency and cache requirements from fighting each other.

## Build

```sh
make
./proxy <port>
./driver.sh          # full autograder; Linux only
```

`proxy.c`, `cache.c`, and `cache.h` are mine. `csapp.c`/`csapp.h`, Tiny, `driver.sh`, and
`nop-server.py` are CS:APP course materials.
