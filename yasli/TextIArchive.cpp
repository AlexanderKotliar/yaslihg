#include "StdAfx.h"
#include <cstdio>
#include <algorithm>
#include "yasli/TextIArchive.h"
#include "yasli/MemoryReader.h"
#include "yasli/MemoryWriter.h"

// #define DEBUG_TEXTIARCHIVE

namespace yasli{

static char hexValueTable[256] = {
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 0, 0, 0, 0, 0, 0,

    0, 10, 11, 12, 13, 14, 15, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 10, 11, 12, 13, 14, 15, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,

    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,

    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

void unescapeString(std::string& dest, const char* begin, const char* end)
{
    dest.resize(end - begin);
    char* ptr = &dest[0];
    while(begin != end){
        if(*begin != '\\'){
            *ptr = *begin;
            ++ptr;
        }
        else{
            ++begin;
            if(begin == end)
                break;

            switch(*begin){
            case '0':  *ptr = '\0'; ++ptr; break;
            case 't':  *ptr = '\t'; ++ptr; break;
            case 'n':  *ptr = '\n'; ++ptr; break;
            case '\\': *ptr = '\\'; ++ptr; break;
            case '\"': *ptr = '\"'; ++ptr; break;
            case '\'': *ptr = '\''; ++ptr; break;
            case 'x':
                if(begin + 2 < end){
                    *ptr = (hexValueTable[int(begin[1])] << 4) + hexValueTable[int(begin[2])];
                    ++ptr;
                    begin += 2;
                    break;
                }
            default:
                *ptr = *begin;
                ++ptr;
                break;
            }
        }
        ++begin;
    }
    dest.resize(ptr - &dest[0]);
}

// ---------------------------------------------------------------------------

class YASLI_API Tokenizer{
public:
    Tokenizer();

    Token operator()(const char* text) const;
private:
    inline bool isSpace(char c) const;
    inline bool isWordPart(unsigned char c) const;
    inline bool isComment(char c) const;
    inline bool isQuoteOpen(int& quoteIndex, char c) const;
    inline bool isQuoteClose(int quoteIndex, char c) const;
    inline bool isQuote(char c) const;
};

Tokenizer::Tokenizer()
{
}

inline bool Tokenizer::isSpace(char c) const
{
	return c == ' ' || c == '\t' || c == '\n' || c == '\r';
}

inline bool Tokenizer::isComment(char c) const
{
	return c == '#';
}


inline bool Tokenizer::isQuote(char c) const
{
	return c == '\"';
}

static const char charTypes[256] = {
	0 /* 0x00: */,
	0 /* 0x01: */,
	0 /* 0x02: */,
	0 /* 0x03: */,
	0 /* 0x04: */,
	0 /* 0x05: */,
	0 /* 0x06: */,
	0 /* 0x07: */,
	0 /* 0x08: */,
	0 /* 0x09: \t */,
	0 /* 0x0A: \n */,
	0 /* 0x0B: */,
	0 /* 0x0C: */,
	0 /* 0x0D: */,
	0 /* 0x0E: */,
	0 /* 0x0F: */,


	0 /* 0x10: */,
	0 /* 0x11: */,
	0 /* 0x12: */,
	0 /* 0x13: */,
	0 /* 0x14: */,
	0 /* 0x15: */,
	0 /* 0x16: */,
	0 /* 0x17: */,
	0 /* 0x18: */,
	0 /* 0x19: */,
	0 /* 0x1A: */,
	0 /* 0x1B: */,
	0 /* 0x1C: */,
	0 /* 0x1D: */,
	0 /* 0x1E: */,
	0 /* 0x1F: */,


	0 /* 0x20:   */,
	0 /* 0x21: ! */,
	0 /* 0x22: " */,
	0 /* 0x23: # */,
	0 /* 0x24: $ */,
	0 /* 0x25: % */,
	0 /* 0x26: & */,
	0 /* 0x27: ' */,
	0 /* 0x28: ( */,
	0 /* 0x29: ) */,
	0 /* 0x2A: * */,
	0 /* 0x2B: + */,
	0 /* 0x2C: , */,
	1 /* 0x2D: - */,
	1 /* 0x2E: . */,
	0 /* 0x2F: / */,


	1 /* 0x30: 0 */,
	1 /* 0x31: 1 */,
	1 /* 0x32: 2 */,
	1 /* 0x33: 3 */,
	1 /* 0x34: 4 */,
	1 /* 0x35: 5 */,
	1 /* 0x36: 6 */,
	1 /* 0x37: 7 */,
	1 /* 0x38: 8 */,
	1 /* 0x39: 9 */,
	0 /* 0x3A: : */,
	0 /* 0x3B: ; */,
	0 /* 0x3C: < */,
	0 /* 0x3D: = */,
	0 /* 0x3E: > */,
	0 /* 0x3F: ? */,


	0 /* 0x40: @ */,
	1 /* 0x41: A */,
	1 /* 0x42: B */,
	1 /* 0x43: C */,
	1 /* 0x44: D */,
	1 /* 0x45: E */,
	1 /* 0x46: F */,
	1 /* 0x47: G */,
	1 /* 0x48: H */,
	1 /* 0x49: I */,
	1 /* 0x4A: J */,
	1 /* 0x4B: K */,
	1 /* 0x4C: L */,
	1 /* 0x4D: M */,
	1 /* 0x4E: N */,
	1 /* 0x4F: O */,


	1 /* 0x50: P */,
	1 /* 0x51: Q */,
	1 /* 0x52: R */,
	1 /* 0x53: S */,
	1 /* 0x54: T */,
	1 /* 0x55: U */,
	1 /* 0x56: V */,
	1 /* 0x57: W */,
	1 /* 0x58: X */,
	1 /* 0x59: Y */,
	1 /* 0x5A: Z */,
	0 /* 0x5B: [ */,
	0 /* 0x5C: \ */,
	0 /* 0x5D: ] */,
	0 /* 0x5E: ^ */,
	1 /* 0x5F: _ */,


	0 /* 0x60: ` */,
	1 /* 0x61: a */,
	1 /* 0x62: b */,
	1 /* 0x63: c */,
	1 /* 0x64: d */,
	1 /* 0x65: e */,
	1 /* 0x66: f */,
	1 /* 0x67: g */,
	1 /* 0x68: h */,
	1 /* 0x69: i */,
	1 /* 0x6A: j */,
	1 /* 0x6B: k */,
	1 /* 0x6C: l */,
	1 /* 0x6D: m */,
	1 /* 0x6E: n */,
	1 /* 0x6F: o */,


	1 /* 0x70: p */,
	1 /* 0x71: q */,
	1 /* 0x72: r */,
	1 /* 0x73: s */,
	1 /* 0x74: t */,
	1 /* 0x75: u */,
	1 /* 0x76: v */,
	1 /* 0x77: w */,
	1 /* 0x78: x */,
	1 /* 0x79: y */,
	1 /* 0x7A: z */,
	0 /* 0x7B: { */,
	0 /* 0x7C: | */,
	0 /* 0x7D: } */,
	0 /* 0x7E: ~ */,
	0 /* 0x7F: */,


	0 /* 0x80: */,
	0 /* 0x81: */,
	0 /* 0x82: */,
	0 /* 0x83: */,
	0 /* 0x84: */,
	0 /* 0x85: */,
	0 /* 0x86: */,
	0 /* 0x87: */,
	0 /* 0x88: */,
	0 /* 0x89: */,
	0 /* 0x8A: */,
	0 /* 0x8B: */,
	0 /* 0x8C: */,
	0 /* 0x8D: */,
	0 /* 0x8E: */,
	0 /* 0x8F: */,


	0 /* 0x90: */,
	0 /* 0x91: */,
	0 /* 0x92: */,
	0 /* 0x93: */,
	0 /* 0x94: */,
	0 /* 0x95: */,
	0 /* 0x96: */,
	0 /* 0x97: */,
	0 /* 0x98: */,
	0 /* 0x99: */,
	0 /* 0x9A: */,
	0 /* 0x9B: */,
	0 /* 0x9C: */,
	0 /* 0x9D: */,
	0 /* 0x9E: */,
	0 /* 0x9F: */,


	0 /* 0xA0: */,
	0 /* 0xA1: Ў */,
	0 /* 0xA2: ў */,
	0 /* 0xA3: Ј */,
	0 /* 0xA4: ¤ */,
	0 /* 0xA5: Ґ */,
	0 /* 0xA6: ¦ */,
	0 /* 0xA7: § */,
	0 /* 0xA8: Ё */,
	0 /* 0xA9: © */,
	0 /* 0xAA: Є */,
	0 /* 0xAB: « */,
	0 /* 0xAC: ¬ */,
	0 /* 0xAD: ­ */,
	0 /* 0xAE: ® */,
	0 /* 0xAF: Ї */,


	0 /* 0xB0: ° */,
	0 /* 0xB1: ± */,
	0 /* 0xB2: І */,
	0 /* 0xB3: і */,
	0 /* 0xB4: ґ */,
	0 /* 0xB5: µ */,
	0 /* 0xB6: ¶ */,
	0 /* 0xB7: · */,
	0 /* 0xB8: ё */,
	0 /* 0xB9: № */,
	0 /* 0xBA: є */,
	0 /* 0xBB: » */,
	0 /* 0xBC: ј */,
	0 /* 0xBD: Ѕ */,
	0 /* 0xBE: ѕ */,
	0 /* 0xBF: ї */,


	0 /* 0xC0: А */,
	0 /* 0xC1: Б */,
	0 /* 0xC2: В */,
	0 /* 0xC3: Г */,
	0 /* 0xC4: Д */,
	0 /* 0xC5: Е */,
	0 /* 0xC6: Ж */,
	0 /* 0xC7: З */,
	0 /* 0xC8: И */,
	0 /* 0xC9: Й */,
	0 /* 0xCA: К */,
	0 /* 0xCB: Л */,
	0 /* 0xCC: М */,
	0 /* 0xCD: Н */,
	0 /* 0xCE: О */,
	0 /* 0xCF: П */,


	0 /* 0xD0: Р */,
	0 /* 0xD1: С */,
	0 /* 0xD2: Т */,
	0 /* 0xD3: У */,
	0 /* 0xD4: Ф */,
	0 /* 0xD5: Х */,
	0 /* 0xD6: Ц */,
	0 /* 0xD7: Ч */,
	0 /* 0xD8: Ш */,
	0 /* 0xD9: Щ */,
	0 /* 0xDA: Ъ */,
	0 /* 0xDB: Ы */,
	0 /* 0xDC: Ь */,
	0 /* 0xDD: Э */,
	0 /* 0xDE: Ю */,
	0 /* 0xDF: Я */,


	0 /* 0xE0: а */,
	0 /* 0xE1: б */,
	0 /* 0xE2: в */,
	0 /* 0xE3: г */,
	0 /* 0xE4: д */,
	0 /* 0xE5: е */,
	0 /* 0xE6: ж */,
	0 /* 0xE7: з */,
	0 /* 0xE8: и */,
	0 /* 0xE9: й */,
	0 /* 0xEA: к */,
	0 /* 0xEB: л */,
	0 /* 0xEC: м */,
	0 /* 0xED: н */,
	0 /* 0xEE: о */,
	0 /* 0xEF: п */,


	0 /* 0xF0: р */,
	0 /* 0xF1: с */,
	0 /* 0xF2: т */,
	0 /* 0xF3: у */,
	0 /* 0xF4: ф */,
	0 /* 0xF5: х */,
	0 /* 0xF6: ц */,
	0 /* 0xF7: ч */,
	0 /* 0xF8: ш */,
	0 /* 0xF9: щ */,
	0 /* 0xFA: ъ */,
	0 /* 0xFB: ы */,
	0 /* 0xFC: ь */,
	0 /* 0xFD: э */,
	0 /* 0xFE: ю */,
	0 /* 0xFF: я */
};

inline bool Tokenizer::isWordPart(unsigned char c) const
{
		return charTypes[c] != 0;
}

Token Tokenizer::operator()(const char* ptr) const
{
	while(isSpace(*ptr))
		++ptr;
	Token cur(ptr, ptr);
	while(!cur && *ptr != '\0'){
		while(isComment(*cur.end)){
#ifdef DEBUG_TOKENIZER
			const char* commentStart = ptr;
#endif
			while(*cur.end && *cur.end != '\n')
				++cur.end;
			while(isSpace(*cur.end))
				++cur.end;
#ifdef DEBUG_TOKENIZER
			std::cout << "Got comment: '" << std::string(commentStart, cur.end) << "'" << std::endl;
#endif
			cur.start = cur.end;
		}
		ASSERT(!isSpace(*cur.end));
		if(isQuote(*cur.end)){
			++cur.end;
			while(*cur.end){ 
				if(*cur.end == '\\'){
					++cur.end;
					if(*cur.end ){
						if(*cur.end != 'x' && *cur.end != 'X')
							++cur.end;
						else{
							++cur.end;
							if(*cur.end)
								++cur.end;
						}
					}
				}
				if(isQuote(*cur.end)){
					++cur.end;
#ifdef DEBUG_TOKENIZER
					std::cout << "Tokenizer result: " << cur.str() << std::endl;
#endif
					return cur;
				}
				else
					++cur.end;
			}
		}
		else{
			//ASSERT(*cur.end);
			if(!*cur.end)
				return cur;

#ifdef DEBUG_TOKENIZER
			char twoChars[] = { *cur.end, '\0' };
			std::cout << twoChars << std::endl;
#endif
			if(isWordPart(*cur.end))
			{
				do{
					++cur.end;
				} while(isWordPart(*cur.end) != 0);
			}
			else
			{
				++cur.end;
				return cur;
			}
#ifdef DEBUG_TOKENIZER
			std::cout << "Tokenizer result: " << cur.str() << std::endl;
#endif
			return cur;
		}
	}
#ifdef DEBUG_TOKENIZER
	std::cout << "Tokenizer result: " << cur.str() << std::endl;
#endif
	return cur;
}


// ---------------------------------------------------------------------------

TextIArchive::TextIArchive()
: Archive(true)
{
}

TextIArchive::~TextIArchive()
{
	stack_.clear();
	reader_.reset();
}

bool TextIArchive::open(const char* buffer, size_t length, bool free)
{
	if(buffer)
		reader_.reset(new MemoryReader(buffer, length, free));

	token_ = Token(reader_->begin(), reader_->begin());
	stack_.clear();

	stack_.push_back(Level());
	readToken();
	putToken();
	stack_.back().start = token_.end;
	return true;
}


bool TextIArchive::load(const char* filename)
{
	if(std::FILE* file = fopen(filename, "rb"))
	{
		std::fseek(file, 0, SEEK_END);
		long fileSize = std::ftell(file);
		std::fseek(file, 0, SEEK_SET);

		void* buffer = 0;
		if(fileSize > 0){
			buffer = std::malloc(fileSize + 1);
			ASSERT(buffer);
			memset(buffer, 0, fileSize + 1);
			long elementsRead = std::fread(buffer, fileSize, 1, file);
			ASSERT(((char*)(buffer))[fileSize] == '\0');
			if(elementsRead != 1){
				return false;
			}
		}
		std::fclose(file);

		filename_ = filename;
		return open((char*)buffer, fileSize, true);
	}
	else{
		return false;
	}
}

void TextIArchive::readToken()
{
	Tokenizer tokenizer;
	token_ = tokenizer(token_.end);
#ifdef DEBUG_TEXTIARCHIVE
	std::cout << " ~ read token '" << token_.str() << "' at " << token_.start - reader_->begin() << std::endl ;
	if(token_){
		ASSERT(token_.start < reader_->end());
		ASSERT(token_.start[0] != '\001');
	}
#endif
}

void TextIArchive::putToken()
{
#ifdef DEBUG_TEXTIARCHIVE
    std::cout << " putToken: \'" << token_.str() << "\'" << std::endl;
#endif
    token_ = Token(token_.start, token_.start);
}

int TextIArchive::line(const char* position) const
{
	return std::count(reader_->begin(), position, '\n') + 1;
}

bool TextIArchive::isName(Token token) const
{
	if(!token)
		return false;
	char firstChar = token.start[0];
	ASSERT(firstChar != '\0');
	ASSERT(firstChar != ' ');
	ASSERT(firstChar != '\n');
    if((firstChar >= 'a' && firstChar <= 'z') ||
       (firstChar >= 'A' && firstChar <= 'Z'))
	{
		size_t len = token_.length();
		if(len == 4)
		{
			if((token_ == "true") ||
			   (token_ == "True") ||
			   (token_ == "TRUE"))
				return false;
		}
		else if(len == 5)
		{
			if((token_ == "false") ||
			   (token_ == "False") ||
			   (token_ == "FALSE"))
				return false;
		}
		return true;
	}
    return false;
}


void TextIArchive::expect(char token)
{
    if(token_ != token){
        MemoryWriter msg;
        msg << "Error parsing file, expected '=' at line " << line(token_.start);
		ASSERT_STR(0, msg.c_str());
    }
}

void TextIArchive::skipBlock()
{
#ifdef DEBUG_TEXTIARCHIVE
    std::cout << "Skipping block from " << token_.end - reader_->start << " ..." << std::endl;
#endif
    if(openBracket() || openContainerBracket())
        closeBracket(); // Skipping entire block
    else
        readToken(); // Skipping value
#ifdef DEBUG_TEXTIARCHIVE
    std::cout << "...till " << token_.end - reader_->start << std::endl;
#endif
}

bool TextIArchive::findName(const char* name)
{

#ifdef DEBUG_TEXTIARCHIVE
    std::cout << " * finding name '" << name << "'" << std::endl;
    std::cout << "   started at byte " << int(token_.start - reader_->start) << std::endl;
#endif
    ASSERT(!stack_.empty());
    const char* start = 0;
    const char* blockBegin = stack_.back().start;
	if(*blockBegin == '\0')
		return false;

    readToken();
    if(!token_){
	    start = blockBegin;
        token_.set(blockBegin, blockBegin);
		readToken();
	}

    if(name[0] == '\0'){
        if(isName(token_)){
#ifdef DEBUG_TEXTIARCHIVE
            std::cout << "Token: '" << token_.str() << "'" << std::endl;
#endif

            start = token_.start;
            readToken();
            expect('=');
            skipBlock();
        }
        else{
			if(token_ == ']' || token_ == '}'){ // CONVERSION
#ifdef DEBUG_TEXTIARCHIVE
                std::cout << "Got close bracket..." << std::endl;
#endif
                putToken();
                return false;
            }
            else{
#ifdef DEBUG_TEXTIARCHIVE
                std::cout << "Got unnamed value: '" << token_.str() << "'" << std::endl;
#endif
                putToken();
                return true;
            }
        }
    }
    else{
        if(isName(token_)){
#ifdef DEBUG_TEXTIARCHIVE
            std::cout << "Seems to be a name '" << token_.str() << "'" << std::endl ;
#endif
            if(token_ == name){
                readToken();
                expect('=');
#ifdef DEBUG_TEXTIARCHIVE
                std::cout << "Got one" << std::endl;
#endif
                return true;
            }
            else{
                start = token_.start;

                readToken();
                expect('=');
                skipBlock();
            }
        }
        else{
            start = token_.start;
			if(token_ == ']' || token_ == '}') // CONVERSION
                token_ = Token(blockBegin, blockBegin);
            else{
                putToken();
                skipBlock();
            }
        }
    }

    while(true){
        readToken();
		if(!token_){
			token_.set(blockBegin, blockBegin);
			continue;
		}
            //return false; // Reached end of file while searching for name
#ifdef DEBUG_TEXTIARCHIVE
        std::cout << "'" << token_.str() << "'" << std::endl;
        std::cout << "Checking for loop: " << token_.start - reader_->start << " and " << start - reader_->begin() << std::endl;
#endif
        ASSERT(start);
        if(token_.start == start){
            putToken();
#ifdef DEBUG_TEXTIARCHIVE
            std::cout << "unable to find..." << std::endl;
#endif
            return false; // Reached a full circle: unable to find name
        }

		if(token_ == '}' || token_ == ']'){ // CONVERSION
#ifdef DEBUG_TEXTIARCHIVE
            std::cout << "Going to begin of block, from " << token_.start - reader_->begin();
#endif
            token_ = Token(blockBegin, blockBegin);
#ifdef DEBUG_TEXTIARCHIVE
            std::cout << " to " << token_.start - reader_->begin() << std::endl;
#endif
            continue; // Reached '}' or ']' while searching for name, continue from begin of block
        }

        if(name[0] == '\0'){
            if(isName(token_)){
                readToken();
                if(!token_)
                    return false; // Reached end of file while searching for name
                expect('=');
                skipBlock();
            }
            else{
                putToken(); // Not a name - put it back
                return true;
            }
        }
        else{
            if(isName(token_)){
                Token nameToken = token_; // token seems to be a name
                readToken();
                expect('=');
                if(nameToken == name)
                    return true; // Success! we found our name
                else
                    skipBlock();
            }
            else{
                putToken();
                skipBlock();
            }
        }

    }

    return false;
}

bool TextIArchive::openBracket()
{
	readToken();
	if(token_ != '{'){
        putToken();
        return false;
    }
    return true;
}

bool TextIArchive::closeBracket()
{
    int relativeLevel = 0;
    while(true){
        readToken();

        if(!token_){
            MemoryWriter msg;
            ASSERT(!stack_.empty());
            const char* start = stack_.back().start;
            msg << filename_.c_str() << ": " << line(start) << " line";
            msg << ": End of file while no matching bracket found";
			ASSERT_STR(0, msg.c_str());
			return false;
        }
		else if(token_ == '}' || token_ == ']'){ // CONVERSION
            if(relativeLevel == 0)
				return token_ == '}';
            else
                --relativeLevel;
        }
		else if(token_ == '{' || token_ == '['){ // CONVERSION
            ++relativeLevel;
        }
    }
    return false;
}

bool TextIArchive::openContainerBracket()
{
	readToken();
	if(token_ != '['){
		putToken();
		return false;
	}
	return true;
}

bool TextIArchive::closeContainerBracket()
{
    readToken();
    if(token_ == ']'){
#ifdef DEBUG_TEXTIARCHIVE
        std::cout << "closeContainerBracket(): ok" << std::endl;
#endif
        return true;
    }
    else{
#ifdef DEBUG_TEXTIARCHIVE
        std::cout << "closeContainerBracket(): failed ('" << token_.str() << "')" << std::endl;
#endif
        putToken();
        return false;
    }
}

bool TextIArchive::operator()(const Serializer& ser, const char* name, const char* label)
{
    if(findName(name)){
        if(openBracket()){
            stack_.push_back(Level());
            stack_.back().start = token_.end;
            ser(*this);
            ASSERT(!stack_.empty());
            stack_.pop_back();
            bool closed = closeBracket();
            ASSERT(closed);
            return true;
        }
    }
    return false;
}

bool TextIArchive::operator()(ContainerSerializationInterface& ser, const char* name, const char* label)
{
    if(findName(name)){
        if(openContainerBracket()){
            stack_.push_back(Level());
            stack_.back().start = token_.end;

            std::size_t size = ser.size();
            std::size_t index = 0;

            while(true){
                readToken();
				if(token_ == '}')
				{
          ASSERT(0 && "Syntax error: closing container with '}'");
					return false;
				}
                if(token_ == ']')
                    break;
                else if(!token_)
				{
                    ASSERT(0 && "Reached end of file while reading container!");
					return false;
				}
                putToken();
                if(index == size)
                    size = index + 1;
                if(index < size){
                    ser(*this, "", "");
                }
                else{
                    skipBlock();
                }
                ser.next();
				++index;
            }
            if(size > index)
                ser.resize(index);

            ASSERT(!stack_.empty());
            stack_.pop_back();
            return true;
        }
    }
    return false;
}

void TextIArchive::checkValueToken()
{
    if(!token_){
        ASSERT(!stack_.empty());
        MemoryWriter msg;
        const char* start = stack_.back().start;
        msg << filename_.c_str() << ": " << line(start) << " line";
        msg << ": End of file while reading element's value";
        ASSERT_STR(0, msg.c_str());
    }
}

bool TextIArchive::checkStringValueToken()
{
    if(!token_){
        return false;
        MemoryWriter msg;
        const char* start = stack_.back().start;
        msg << filename_.c_str() << ": " << line(start) << " line";
        msg << ": End of file while reading element's value";
        ASSERT_STR(0, msg.c_str());
		return false;
    }
    if(token_.start[0] != '"' || token_.end[-1] != '"'){
        return false;
        MemoryWriter msg;
        const char* start = stack_.back().start;
        msg << filename_.c_str() << ": " << line(start) << " line";
        msg << ": Expected string";
        ASSERT_STR(0, msg.c_str());
		return false;
    }
    return true;
}

bool TextIArchive::operator()(int& value, const char* name, const char* label)
{
    if(findName(name)){
        readToken();
        checkValueToken();
        value = strtol(token_.start, 0, 10);
        return true;
    }
    return false;
}

bool TextIArchive::operator()(unsigned int& value, const char* name, const char* label)
{
    if(findName(name)){
        readToken();
        checkValueToken();
        value = strtoul(token_.start, 0, 10);
        return true;
    }
    return false;
}


bool TextIArchive::operator()(short& value, const char* name, const char* label)
{
    if(findName(name)){
        readToken();
        checkValueToken();
        value = (short)strtol(token_.start, 0, 10);
        return true;
    }
    return false;
}

bool TextIArchive::operator()(unsigned short& value, const char* name, const char* label)
{
    if(findName(name)){
        readToken();
        checkValueToken();
        value = (unsigned short)strtoul(token_.start, 0, 10);
        return true;
    }
    return false;
}

bool TextIArchive::operator()(__int64& value, const char* name, const char* label)
{
    if(findName(name)){
        readToken();
        checkValueToken();
#ifdef WIN32
		value = _strtoui64(token_.start, 0, 10);
#else
#endif
        return true;
    }
    return false;
}

bool TextIArchive::operator()(float& value, const char* name, const char* label)
{
    if(findName(name)){
        readToken();
        checkValueToken();
#ifdef _MSC_VER
        value = float(std::atof(token_.str().c_str()));
#else
        value = std::strtof(token_.start, 0);
#endif
        return true;
    }
    return false;
}

bool TextIArchive::operator()(double& value, const char* name, const char* label)
{
    if(findName(name)){
        readToken();
        checkValueToken();
#ifdef _MSC_VER
        value = std::atof(token_.str().c_str());
#else
        value = std::strtof(token_.start, 0);
#endif
        return true;
    }
    return false;
}

bool TextIArchive::operator()(std::string& value, const char* name, const char* label)
{
    if(findName(name)){
        readToken();
        if(checkStringValueToken())
			unescapeString(value, token_.start + 1, token_.end - 1);
		else
			return false;
        return true;
    }
    return false;
}

bool TextIArchive::operator()(bool& value, const char* name, const char* label)
{
    if(findName(name)){
        readToken();
        checkValueToken();
        if(token_ == "true" || token_ == "True" || token_ == "TRUE")
            value = true;
        else
            value = false;
        return true;
    }
    return false;
}

bool TextIArchive::operator()(signed char& value, const char* name, const char* label)
{
    if(findName(name)){
        readToken();
        checkValueToken();
        value = (signed char)strtol(token_.start, 0, 10);
        return true;
    }
    return false;
}

bool TextIArchive::operator()(unsigned char& value, const char* name, const char* label)
{
    if(findName(name)){
        readToken();
        checkValueToken();
        value = (unsigned char)strtol(token_.start, 0, 10);
        return true;
    }
    return false;
}

bool TextIArchive::operator()(char& value, const char* name, const char* label)
{
    if(findName(name)){
        readToken();
        checkValueToken();
        value = (char)strtol(token_.start, 0, 10);
        return true;
    }
    return false;
}

bool TextIArchive::operator()(StringListStaticValue& value, const char* name, const char* label)
{
    if(findName(name)){
        readToken();
        if(checkStringValueToken()){
            Token tok(token_.start + 1, token_.end - 1);
            ASSERT(token_.start < token_.end);
            ASSERT(tok.start < tok.end);
            std::string str = tok.str();
#ifdef DEBUG_TEXTIARCHIVE
            std::cout << "Reading '" << str << "'" << std::endl;
#endif

            int index = value.stringList().find(str.c_str());
            if(index != StringListStatic::npos){
                value = index;
                return true;
            }
            else{
                warning("Unable to read StringListStaticValue, no such value");
                return false;
            }

        }
        else
            return false;
    }
    return false;
}

}
