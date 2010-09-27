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

  bool operator < (const Card& that) const { return value < that.value; }
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

unsigned int ScoreLowHand(const std::vector<Card>& cards, const Combinations& combo);
unsigned int ScoreHighHand(const std::vector<Card>& cards, const Combinations& combo);

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


bool CompareCardsDescending(const Card& a, const Card& b)
{
  return a.value > b.value;
}


bool CompareCountsDescending(const std::pair<unsigned int, unsigned int>& a,
                             const std::pair<unsigned int, unsigned int>& b)
{
  return (a.second > b.second) || (a.second == b.second && a.first > b.first);
}


unsigned int ScoreLowHand(const std::vector<Card>& cards, const Combinations& combo)
{
  const Card& a = cards[combo.current()[0]];
  const Card& b = cards[combo.current()[1]];

  unsigned int score = (a.value >= b.value) ?
    (a.value * 15 + b.value) : (b.value * 15 + a.value);
  if (a.value == b.value)
    score += 1000;
  return score;
}


unsigned int ScoreHighHand(const std::vector<Card>& cards, const Combinations& combo)
{
  // This function assumes that there are 7 cards in the cards vector and that
  // the combo describes the low hand, i.e. the two cards which AREN'T in this
  // hand.

  // Get the current hand.
  Card hand[5];
  unsigned int index = 0;
  for (unsigned int i = 0; i < cards.size(); ++i) {
    if (i != combo.current()[0] && i != combo.current()[1])
      hand[index++] = cards[i];
  }
  std::sort(hand, hand + 5, CompareCardsDescending);

  // Analyse the hand, looking for the number of groups, the largest group and the sizes of each group.
  unsigned int largestGroup = 0;
  std::vector< std::pair<unsigned int, unsigned int> > counts;
  counts.reserve(5);
  index = 0;
  while (index < 5) {
    unsigned int j = index + 1;
    while (j < 5 && hand[index].value == hand[j].value)
      ++j;

    counts.push_back(std::make_pair(hand[index].value, j - index));
    if (counts[index].second > largestGroup)
      largestGroup = counts[index].second;

    index = j;
  }
  std::sort(counts.begin(), counts.end(), CompareCountsDescending);

  // Calculate a base score for the hand using the sorted groups of cards.
  unsigned int score = 0;
  for (unsigned int i = 0; i < counts.size(); ++i)
    score = score * 15 + counts[i].first;

  // Categorise the hand and adjust the score to account for the category.
  bool isStraight = true;
  bool isFlush = true;
  switch (counts.size()) {
    case 5:
      for (unsigned int i = 1; i < 5 && isStraight; ++i)
        isStraight = (hand[i].value + i) == hand[0].value;
      for (unsigned int i = 1; i < 5 && isFlush; ++i)
        isFlush = hand[i].suit == hand[0].suit;

      if (isStraight && isFlush)
        score += 8000000; // Straight flush
      else if (isFlush)
        score += 5000000; // Flush
      else if (isStraight)
        score += 4000000; // Straight

      break;

    case 4:
      score += 1000000; // One pair
      break;

    case 3:
      if (largestGroup == 3)
        score += 3000000; // Three of a kind
      else
        score += 2000000; // Two pairs
      break;

    case 2:
      if (largestGroup == 4)
        score += 7000000; // Four of a kind
      else
        score += 6000000; // Full house
      break;

    default:
      return 0; // This should be impossible.
  }

  return score;
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
      printf("\tScore: %4u, %7u\n", ScoreLowHand(g->playerCards, combo), ScoreHighHand(g->playerCards, combo));
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

