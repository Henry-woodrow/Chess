#!/usr/bin/env python3
"""
Query OpenAI's ChatGPT for a chess move.

The script reads a simple board description as its first argument and expects
an API key in the OPENAI_API_KEY environment variable. It prints a move in the
format "sr sc er ec" to stdout. If anything goes wrong, a fallback move of
"0 0 0 0" is returned.
"""
import os
import sys


def main() -> None:
    board = sys.argv[1] if len(sys.argv) > 1 else ""
    api_key = os.getenv("OPENAI_API_KEY")
    if not api_key:
        print("0 0 0 0")
        return
    try:
        import openai  # type: ignore
    except Exception:
        print("0 0 0 0")
        return
    openai.api_key = api_key
    prompt = (
        "You are a chess engine playing black. Given the board state: "
        + board
        + "\nReturn four integers 'sr sc er ec' for your move."
    )
    try:
        response = openai.ChatCompletion.create(
            model="gpt-3.5-turbo",
            messages=[
                {"role": "system", "content": "You suggest legal chess moves for black and respond with four integers."},
                {"role": "user", "content": prompt},
            ],
            max_tokens=10,
            temperature=0,
        )
        move = response["choices"][0]["message"]["content"].strip()
        print(move)
    except Exception:
        print("0 0 0 0")


if __name__ == "__main__":
    main()
