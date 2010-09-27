#include <algorithm>
#include <cstdlib>
#include <cstdio>
#include <vector>

#include <tbb/blocked_range.h>
#include <tbb/parallel_for.h>
#include <tbb/parallel_reduce.h>
#include <tbb/tick_count.h>


//
// Types
//

class Combinations
{
public:
  Combinations(unsigned int total, unsigned int num);
  ~Combinations();

  bool next();
  const unsigned int* current() const;

  void print() const;

private:
  const unsigned int _kTotal;
  const unsigned int _kNum;
  const unsigned int _kBaseMax;

  unsigned int* _combination;
};


struct Card
{
  unsigned int value;
  unsigned int suit;
};


struct Game
{
  std::vector<Card> playerCards;
  std::vector<Card> dealerCards;
};


//
// Forward declarations
//

Card ParseCard(const char* card);
std::vector<Card> ParseHand(char* line);
std::vector<Game> ParseGames(FILE* f);

void PrintCard(const Card& card);
void PrintHand(const std::vector<Card>& cards);
void PrintGames(const std::vector<Game>& games);


//
// Combinations methods
//

Combinations::Combinations(unsigned int total, unsigned int num) :
  _kTotal(total),
  _kNum(num),
  _kBaseMax(total - num)
{
  _combination = new unsigned int[_kNum];

  for (unsigned int i = 0; i < _kNum; ++i)
    _combination[i] = i;
}


Combinations::~Combinations()
{
  delete[] _combination;
}


bool Combinations::next()
{
  int i = _kNum - 1;
  while (i >= 0 && _combination[i] == _kBaseMax + i)
    --i;

  if (i < 0)
    return false; // We've run out of combinations.

  ++_combination[i];
  ++i;
  while (i < (int)_kNum) {
    _combination[i] = _combination[i - 1] + 1;
    ++i;
  }

  return true;
}


const unsigned int* Combinations::current() const
{
  return _combination;
}


void Combinations::print() const
{
  printf("%3u", _combination[0]);
  for (unsigned int pos = 1; pos < _kNum; ++pos)
    printf(" %3u", _combination[pos]);
  printf("\n");
}


//
// Functions
//

Card ParseCard(const char* card)
{
  Card c;
  switch (card[0]) {
    case '2': c.value =  2; break;
    case '3': c.value =  3; break;
    case '4': c.value =  4; break;
    case '5': c.value =  5; break;
    case '6': c.value =  6; break;
    case '7': c.value =  7; break;
    case '8': c.value =  8; break;
    case '9': c.value =  9; break;
    case 'X': c.value = 10; break;
    case 'J': c.value = 11; break;
    case 'Q': c.value = 12; break;
    case 'K': c.value = 13; break;
    case 'A': c.value = 14; break;
    default: c.value = 0; break;
  }
  c.suit = card[1];
  return c;
}


std::vector<Card> ParseHand(char* line)
{
  int len = strlen(line);
  if (line[len-1] == '\n')
    line[len-1] = '\0';

  std::vector<Card> cards;
  for (const char* card = line; *card != '\0'; card += 2)
    cards.push_back(ParseCard(card));

  return cards;
}


std::vector<Game> ParseGames(FILE* f)
{
  std::vector<Game> games;
  char line[256];
  while (!feof(f)) {
    Game game;
    if (!fgets(line, 256, f))
      break;
    game.playerCards = ParseHand(line);
    if (!fgets(line, 256, f))
      break;
    game.dealerCards = ParseHand(line);
    games.push_back(game);
  }
  return games;
}


void PrintCard(const Card& card)
{
  char value;
  switch (card.value) {
    case  2: value = '2'; break;
    case  3: value = '3'; break;
    case  4: value = '4'; break;
    case  5: value = '5'; break;
    case  6: value = '6'; break;
    case  7: value = '7'; break;
    case  8: value = '8'; break;
    case  9: value = '9'; break;
    case 10: value = 'X'; break;
    case 11: value = 'J'; break;
    case 12: value = 'Q'; break;
    case 13: value = 'K'; break;
    case 14: value = 'A'; break;
    default: value = '_'; break;
  }
  printf("%c%c", value, card.suit);
}


void PrintHand(const std::vector<Card>& cards)
{
  std::vector<Card>::const_iterator c;
  for (c = cards.begin(); c != cards.end(); ++c) {
    printf(" ");
    PrintCard(*c);
  }
}


void PrintGames(const std::vector<Game>& games)
{
  std::vector<Game>::const_iterator g;
  for (g = games.begin(); g != games.end(); ++g) {
    printf("Player:");
    PrintHand(g->playerCards);
    printf("\n");

    Combinations combo(g->playerCards.size(), 2);
    do {
      const unsigned int* indexes = combo.current();
      PrintCard(g->playerCards[indexes[0]]);
      printf(" ");
      PrintCard(g->playerCards[indexes[1]]);
      printf(" |");
      for (unsigned int i = 0; i < g->playerCards.size(); ++i) {
        if (i != indexes[0] && i != indexes[1]) {
          printf(" ");
          PrintCard(g->playerCards[i]);
        }
      }
      printf("\n");
    } while (combo.next());
    printf("Dealer:");
    PrintHand(g->dealerCards);
    printf("\n");
    printf("----\n");
  }
}


int main(int argc, char** argv)
{
  if (argc != 2) {
    fprintf(stderr, "Usage: %s <infile>\n", argv[0]);
    return -1;
  }

  // Start timing.
  tbb::tick_count startTime = tbb::tick_count::now();
  
  // Parse the games.
  FILE* f = fopen(argv[1], "r");
  if (!f) {
    fprintf(stderr, "Error: %s\n", strerror(ferror(f)));
    return -1;
  }
  std::vector<Game> games = ParseGames(f);
  fclose(f);

  // Stop timing.
  tbb::tick_count endTime = tbb::tick_count::now();

  // Print the results.
  PrintGames(games);

  return 0;
}

