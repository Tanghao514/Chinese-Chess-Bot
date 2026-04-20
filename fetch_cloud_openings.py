import time
import re
import requests

API = "http://www.chessdb.cn/chessdb.php"
START_FEN = "rnbakabnr/9/1c5c1/p1p1p1p1p/9/9/P1P1P1P1P/1C5C1/9/RNBAKABNR w"
MAX_PLY = 24
OUT_FILE = "movebook_generated.txt"
REQUEST_SLEEP = 0.05
TIMEOUT = 10

query_count = 0
saved_count = 0
start_time = time.time()
results = []

def format_seconds(sec: float) -> str:
    sec = int(sec)
    h = sec // 3600
    m = (sec % 3600) // 60
    s = sec % 60
    if h > 0:
        return f"{h}h{m:02d}m{s:02d}s"
    if m > 0:
        return f"{m}m{s:02d}s"
    return f"{s}s"

def branch_limit(ply: int) -> int:
    if ply < 6:
        return 3
    if ply < 12:
        return 2
    if ply < 18:
        return 2
    return 1

def weight_for_rank(idx: int) -> int:
    if idx == 0:
        return 120
    if idx == 1:
        return 100
    if idx == 2:
        return 85
    return 70

def fen_to_board(fen: str):
    parts = fen.strip().split()
    board_part = parts[0]
    side = parts[1] if len(parts) > 1 else "w"

    rows = board_part.split("/")
    if len(rows) != 10:
        raise ValueError(f"bad fen rows: {fen}")

    board = []
    for row in rows:
        line = []
        for ch in row:
            if ch.isdigit():
                line.extend(["."] * int(ch))
            else:
                line.append(ch)
        if len(line) != 9:
            raise ValueError(f"bad fen row width: {row}")
        board.append(line)
    return board, side

def board_to_fen(board, side: str) -> str:
    rows = []
    for row in board:
        s = []
        empty = 0
        for ch in row:
            if ch == ".":
                empty += 1
            else:
                if empty:
                    s.append(str(empty))
                    empty = 0
                s.append(ch)
        if empty:
            s.append(str(empty))
        rows.append("".join(s))
    return "/".join(rows) + " " + side

def iccs_to_xy(square: str):
    file_c = square[0]
    rank_c = square[1]
    x = ord(file_c) - ord("a")
    y = int(rank_c)
    row = 9 - y
    col = x
    return row, col

def apply_move_to_fen(fen: str, move: str) -> str:
    board, side = fen_to_board(fen)

    sr, sc = iccs_to_xy(move[:2])
    tr, tc = iccs_to_xy(move[2:])

    piece = board[sr][sc]
    if piece == ".":
        raise ValueError(f"source empty for move {move} on fen {fen}")

    board[sr][sc] = "."
    board[tr][tc] = piece

    next_side = "b" if side == "w" else "w"
    return board_to_fen(board, next_side)

def parse_queryall_response(text: str):
    text = text.strip()
    if text in ("unknown", "invalid board", "checkmate", "stalemate", ""):
        return []

    items = []
    for chunk in text.split("|"):
        chunk = chunk.strip()
        if not chunk:
            continue

        m = re.search(r"move:([a-i][0-9][a-i][0-9])", chunk)
        if not m:
            continue

        move = m.group(1)
        rank_m = re.search(r"rank:([-]?\d+)", chunk)
        score_m = re.search(r"score:([-]?\d+)", chunk)
        win_m = re.search(r"winrate:([-]?\d+(?:\.\d+)?)", chunk)

        rank = int(rank_m.group(1)) if rank_m else 10**9
        score = int(score_m.group(1)) if score_m else 0
        winrate = float(win_m.group(1)) if win_m else 0.0

        items.append({
            "move": move,
            "rank": rank,
            "score": score,
            "winrate": winrate,
            "raw": chunk
        })

    items.sort(key=lambda x: (x["rank"], -x["winrate"], -x["score"], x["move"]))
    return items

def queryall(fen: str):
    global query_count
    query_count += 1

    params = {
        "action": "queryall",
        "board": fen,
        "learn": 0,
        "showall": 0
    }

    resp = requests.get(API, params=params, timeout=TIMEOUT)
    resp.raise_for_status()
    return parse_queryall_response(resp.text)

def should_keep(item, local_idx: int) -> bool:
    if item["rank"] > 8:
        return False
    if item["winrate"] < 20 and item["score"] < -150:
        return False
    return True

def dedup_results(items):
    best = {}
    for seq, w in items:
        if seq not in best or w > best[seq]:
            best[seq] = w
    return sorted(best.items(), key=lambda x: (len(x[0].split()), x[0]))

def save_results(final=False):
    global saved_count
    final_items = dedup_results(results)
    with open(OUT_FILE, "w", encoding="utf-8") as f:
        f.write("// generated from chessdb queryall\n")
        f.write("static const std::pair<const char*, int> BOOK[] = {\n")
        for seq, w in final_items:
            f.write(f'    {{"{seq}", {w}}},\n')
        f.write("};\n")
    saved_count = len(final_items)
    tag = "final save" if final else "checkpoint save"
    print(f"[info] {tag}: {saved_count} lines -> {OUT_FILE}", flush=True)

def dfs(fen: str, path: list[str], ply: int):
    if ply >= MAX_PLY:
        return

    try:
        moves = queryall(fen)
    except Exception as e:
        print(f"[warn] query failed | ply={ply} | path={' '.join(path) if path else '(root)'} | err={e}", flush=True)
        return

    elapsed = format_seconds(time.time() - start_time)
    print(f"[query {query_count}] ply={ply}/{MAX_PLY} raw={len(moves)} elapsed={elapsed} path={' '.join(path) if path else '(root)'}", flush=True)

    if not moves:
        return

    kept = []
    limit = branch_limit(ply)
    for idx, item in enumerate(moves):
        if should_keep(item, idx):
            kept.append(item)
        if len(kept) >= limit:
            break

    print(f"[keep ] ply={ply} keep={len(kept)} limit={limit} path={' '.join(path) if path else '(root)'}", flush=True)

    for idx, item in enumerate(kept):
        mv = item["move"]
        new_path = path + [mv]
        w = weight_for_rank(idx)
        results.append((" ".join(new_path), w))

        print(
            f"[move ] ply={ply+1} choose={idx+1}/{len(kept)} move={mv} "
            f"rank={item['rank']} winrate={item['winrate']} score={item['score']} "
            f"total_lines={len(results)}",
            flush=True
        )

        if len(results) % 100 == 0:
            save_results(final=False)

        try:
            next_fen = apply_move_to_fen(fen, mv)
        except Exception as e:
            print(f"[warn] apply move failed | move={mv} | path={' '.join(new_path)} | err={e}", flush=True)
            continue

        time.sleep(REQUEST_SLEEP)
        dfs(next_fen, new_path, ply + 1)

def main():
    print("[info] start fetching cloud openings...", flush=True)
    print(f"[info] max_ply={MAX_PLY}, output={OUT_FILE}", flush=True)

    try:
        dfs(START_FEN, [], 0)
    except KeyboardInterrupt:
        print("\n[info] interrupted by user, saving partial results...", flush=True)
        save_results(final=False)
        return

    save_results(final=True)
    print(f"[info] done, total unique lines = {saved_count}", flush=True)

if __name__ == "__main__":
    main()