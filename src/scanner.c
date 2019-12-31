#include <tree_sitter/parser.h>
#include <string.h>

enum TokenType {
	NULL_CHAR,
	HAS_GROUP_NAME,
	BEGIN_COUNT_QUANTIFIER,
	BEGIN_UNICODE_CODEPOINT,
	BEGIN_UNICODE_PROPERTY,
};

void * tree_sitter_regex_external_scanner_create() { return NULL; }
void tree_sitter_regex_external_scanner_destroy(void *payload) {}
unsigned tree_sitter_regex_external_scanner_serialize(void *payload, char *buffer) { return 0; }
void tree_sitter_regex_external_scanner_deserialize(void *payload, const char *buffer, unsigned length) {}

static void advance(TSLexer *lexer) {
	lexer->advance(lexer, false);
}
static bool checkForLazyQuantifier(TSLexer *lexer) {
	//TODO
}
static bool checkForCountQuantifier(TSLexer *lexer) {
	char digits[] = "0123456789";
	if (lexer->lookahead == 0 || strchr(digits, lexer->lookahead) == NULL) {
		return false;
	}
	while (lexer->lookahead != 0 && strchr(digits, lexer->lookahead) != NULL) {
		advance(lexer);
	}
	if (lexer->lookahead == ',') {
		advance(lexer);
		while (lexer->lookahead != 0 && strchr(digits, lexer->lookahead) != NULL) {
			advance(lexer);
		}
	}
	if (lexer->lookahead == '}') {
		return true;
	}
	return false;
}
static bool checkForUnicodeCodePoint(TSLexer *lexer) {
	char hex[] = "0123456789abcdefABCDEF";
	if (lexer->lookahead == 0 || strchr(hex, lexer->lookahead) == NULL) {
		return false;
	}
	while (lexer->lookahead == '0') {
		advance(lexer);
	}
	int len = 5;
	if (lexer->lookahead == '1') {
		advance(lexer);
		len--;
		if (lexer->lookahead == '0') {
			advance(lexer);
		}
	}
	int i = 0;
	for (; i<len; i++) {
		if (lexer->lookahead == 0) {
			return false;
		}
		if (lexer->lookahead == '}') {
			break;
		}
		if (strchr(hex, lexer->lookahead) == NULL) {
			return false;
		}
		advance(lexer);
	}
	if (i > 0 && lexer->lookahead == '}') {
		return true;
	}
	return false;
}
static bool checkForUnicodeProperty(TSLexer *lexer) {
	//TODO
	return false;
}

bool tree_sitter_regex_external_scanner_scan(
	void *payload,
	TSLexer *lexer,
	const bool *valid_symbols
) {
	if (lexer->lookahead == '\\') {
		lexer->mark_end(lexer);
		advance(lexer);
		if (valid_symbols[NULL_CHAR] && lexer->lookahead == '0') {
			advance(lexer);
			if (lexer->lookahead != 0 && strchr("0123456789", lexer->lookahead) != NULL) {	//0-9
				return false;
			}
			lexer->mark_end(lexer);
			lexer->result_symbol = NULL_CHAR;
			return true;
		}
	}
	else if (valid_symbols[HAS_GROUP_NAME] && lexer->lookahead == '<') {
		lexer->mark_end(lexer);
		advance(lexer);
		if (lexer->lookahead == '>') {
			return false;	// <>
		}
		char word[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789$_";
		char hex[] = "0123456789abcdefABCDEF";
		while (lexer->lookahead != 0 && lexer->lookahead != '>') {
			if (strchr(word, lexer->lookahead) != NULL) {
				 advance(lexer);
			}
			else if (lexer->lookahead == '\\') {
				advance(lexer);
				if(lexer->lookahead != 'u') {
					return false;
				}
				for (int i=0; i<4; i++) {
					advance(lexer);
					if (lexer->lookahead == 0 || strchr(hex, lexer->lookahead) == NULL) {
						return false;
					}
				}
				advance(lexer);
			}
			else{
				return false;
			}
		}
		if(lexer->lookahead == '>') {
			lexer->result_symbol = HAS_GROUP_NAME;
			return true;
		}
	}
	else if (lexer->lookahead == '{') {
		lexer->mark_end(lexer);
		advance(lexer);
		if (valid_symbols[BEGIN_COUNT_QUANTIFIER] && checkForCountQuantifier(lexer)) {
			lexer->result_symbol = BEGIN_COUNT_QUANTIFIER;
			return true;
		}
		else if (valid_symbols[BEGIN_UNICODE_CODEPOINT] && checkForUnicodeCodePoint(lexer)) {
			lexer->result_symbol = BEGIN_UNICODE_CODEPOINT;
			return true;
		}
		else if (valid_symbols[BEGIN_UNICODE_PROPERTY] && checkForUnicodeProperty(lexer)) {
			lexer->result_symbol = BEGIN_UNICODE_PROPERTY;
			return true;
		}
	}
	
	return false;
}
