"""Small dependency-free fuzzy search over manual capture markers."""

from __future__ import annotations

import re
import unicodedata

from baccable_lab.events.catalog import EVENT_GROUPS, SEARCH_ALIASES, label_text


def _normalise(value: str) -> str:
    value = unicodedata.normalize("NFKD", value.casefold())
    value = "".join(char for char in value if not unicodedata.combining(char))
    return " ".join(re.findall(r"[a-z0-9]+", value))


def search_markers(query: str, limit: int = 10) -> list[tuple[str, str]]:
    """Return matching (stable label, visible title), best textual match first."""
    needle = _normalise(query)
    if not needle or limit <= 0:
        return []
    query_tokens = needle.split()
    results: list[tuple[int, str, str]] = []
    for _, entries in EVENT_GROUPS.values():
        for label, title in entries:
            terms = [label_text(label), label.replace("_", " "), *SEARCH_ALIASES.get(label, ())]
            normalised_terms = [_normalise(term) for term in terms]
            normalised_terms = [term for term in normalised_terms if term]
            if needle in normalised_terms:
                rank = 0
            elif any(term.startswith(needle) for term in normalised_terms):
                rank = 1
            elif any(needle in term for term in normalised_terms):
                rank = 2
            elif all(any(token in term for term in normalised_terms) for token in query_tokens):
                rank = 3
            else:
                continue
            results.append((rank, title.casefold(), label))
    results.sort()
    return [(label, label_text(label)) for _, _, label in results[:limit]]
