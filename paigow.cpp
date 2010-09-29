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


struct PlayerHand
{
  unsigned int index;

  unsigned int lowHandScore;
  unsigned int highHandScore;

  unsigned int wins;
  unsigned int draws;
  unsigned int losses;

  PlayerHand() : index(0), lowHandScore(0), highHandScore(0), wins(0), draws(0), losses(0) {}
};


struct Game
{
  std::vector<Card> playerCards;
  std::vector<Card> dealerCards;

  PlayerHand best;
  PlayerHand secondBest;
  PlayerHand worst;

  Game() : playerCards(), dealerCards(), best(), secondBest(), worst() {}
};


//
// Forward declarations
//

Card ParseCard(const char* card);
std::vector<Card> ParseHand(char* line);
std::vector<Game> ParseGames(FILE* f);

bool CompareCardsDescending(const Card& a, const Card& b);
bool CompareCountsDescending(const std::pair<unsigned int, unsigned int>& a,
                             const std::pair<unsigned int, unsigned int>& b);
bool ComparePlayerHandsDescending(const PlayerHand& a, const PlayerHand& b);

unsigned int ScoreLowHand(const std::vector<Card>& cards, const Combinations& combo);
unsigned int ScoreHighHand(const std::vector<Card>& cards, const Combinations& combo);
void PlayGame(Game& game);

void PrintCard(const Card& card);
void PrintHand(const char* title, const std::vector<Card>& cards, const PlayerHand& hand);
void PrintGames(const std::vector<Game>& games,
    const tbb::tick_count& startTime, const tbb::tick_count& endTime);


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
  switch (card[1]) {
    case 'C': c.suit = 0; break;
    case 'D': c.suit = 1; break;
    case 'H': c.suit = 2; break;
    case 'S': c.suit = 3; break;
    default: c.suit = 0; break;
  }
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


bool ComparePlayerHandsDescending(const PlayerHand& a, const PlayerHand& b)
{
  return (a.wins > b.wins) || (a.wins == b.wins && a.draws > b.draws);
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
  unsigned int suits[4] = { 0, 0, 0, 0 };
  for (unsigned int i = 0; i < cards.size(); ++i) {
    if (i != combo.current()[0] && i != combo.current()[1])
      suits[cards[i].suit] |= (1 << cards[i].value);
  }

  unsigned int groups = suits[0] | suits[1] | suits[2] | suits[3];
  unsigned int numGroups = __builtin_popcount(groups);

  switch (numGroups) {
    case 5:
    {
      bool is_flush = (__builtin_popcount(suits[0]) == 5) || (__builtin_popcount(suits[1]) == 5)
        || (__builtin_popcount(suits[2]) == 5) || (__builtin_popcount(suits[3]) == 5);
      bool is_straight = __builtin_ctz(groups) + __builtin_clz(groups) == 27;
      if (is_flush && is_straight)
        return 8000000 + groups;  // Straight flush
      else if (is_flush)
        return 5000000 + groups;  // Flush
      else if (is_straight)
        return 4000000 + groups;  // Straight
      else
        return groups;            // Nothing.
    }

    case 4:
    {
      unsigned int pairs = (suits[0] & suits[1]) | (suits[0] & suits[2]) | (suits[0] & suits[3])
        | (suits[1] & suits[2]) | (suits[1] & suits[3]) | (suits[2] & suits[3]);
      groups &= ~pairs;
      return 1000000 + (__builtin_ctz(pairs) << 14) + groups; // One pair.
    }

    case 3:
    {
      unsigned int pairs = (suits[0] & suits[1]) | (suits[0] & suits[2]) | (suits[0] & suits[3])
        | (suits[1] & suits[2]) | (suits[1] & suits[3]) | (suits[2] & suits[3]);
      groups &= ~pairs;
      if (__builtin_popcount(pairs) > 1) // Two pairs.
        return 2000000 + ((32 - __builtin_clz(pairs)) << 8) + (__builtin_ctz(pairs) << 4) + __builtin_ctz(groups);
      else // Three of a kind
        return 3000000 + (__builtin_ctz(pairs) << 14) + groups;
    }

    case 2:
    {
      unsigned int four = (suits[0] & suits[1] & suits[2] & suits[3]);
      groups &= ~four;
      if (four != 0)
        return 7000000 + (__builtin_ctz(four) << 4) + __builtin_ctz(groups); // Four of a kind

      unsigned int three = (suits[0] & suits[1] & suits[2]) | (suits[0] & suits[1] & suits[3])
        | (suits[0] & suits[2] & suits[3]) | (suits[1] & suits[2] & suits[3]);
      groups &= ~three;
      return 6000000 + (__builtin_ctz(three) << 4) + __builtin_ctz(groups); // Full house
    }

    default:
      return 0; // Should be impossible.
  }
}


void PlayGame(Game& game)
{
  // We assume that the player always has 7 cards.

  // Calculate the scores for all possible player hands.
  Combinations playerCombos(7, 2);
  PlayerHand playerHands[21];
  unsigned int i = 0;
  do {
    playerHands[i].index = i;
    playerHands[i].lowHandScore = ScoreLowHand(game.playerCards, playerCombos);
    playerHands[i].highHandScore = ScoreHighHand(game.playerCards, playerCombos);
    ++i;
  } while (playerCombos.next());


  // For each possible dealer hand, play each possible player hand against it and record the results.
  std::vector<Card> tmpDealerHand(7);
  Combinations dealerCombos(game.dealerCards.size(), 7);
  do {
    for (unsigned int i = 0; i < 7; ++i)
      tmpDealerHand[i] = game.dealerCards[dealerCombos.current()[i]];
    
    Combinations dealerHandCombos(7, 2);
    do {
      unsigned int dealerLowScore = ScoreLowHand(tmpDealerHand, dealerHandCombos);
      unsigned int dealerHighScore = ScoreHighHand(tmpDealerHand, dealerHandCombos);
      for (unsigned int i = 0; i < 21; ++i) {
        unsigned int playerLowScore = playerHands[i].lowHandScore;
        unsigned int playerHighScore = playerHands[i].highHandScore;
        if (playerLowScore > dealerLowScore && playerHighScore > dealerHighScore)
          ++playerHands[i].wins;
        else if (playerLowScore < dealerLowScore && playerHighScore < dealerHighScore)
          ++playerHands[i].losses;
        else
          ++playerHands[i].draws;
      }
    } while (dealerHandCombos.next());
  } while (dealerCombos.next());

  // Find the player hands with the best, second best and worst results.
  std::sort(playerHands, playerHands + 21, ComparePlayerHandsDescending);
  game.best = playerHands[0];
  game.secondBest = playerHands[1];
  game.worst = playerHands[20];
}


void PrintCard(const Card& card)
{
  const char* values = "0123456789XJQKA";
  const char* suits = "CDHS";

  printf("%c%c", values[card.value], suits[card.suit]);
}


void PrintHand(const char* title, const std::vector<Card>& cards, const PlayerHand& hand)
{
  Combinations combo(cards.size(), 2);
  for (unsigned int i = 0; i < hand.index; ++i)
    combo.next();

  printf("%s:\t", title);
  const unsigned int* indexes = combo.current();
  PrintCard(cards[indexes[0]]);
  printf(" ");
  PrintCard(cards[indexes[1]]);
  printf(" |");
  for (unsigned int i = 0; i < cards.size(); ++i) {
    if (i != indexes[0] && i != indexes[1]) {
      printf(" ");
      PrintCard(cards[i]);
    }
  }
  printf("\t\tW: %9u  D: %9u  L: %9u\n", hand.wins, hand.draws, hand.losses);
}


void PrintGames(const std::vector<Game>& games,
    const tbb::tick_count& startTime, const tbb::tick_count& endTime)
{
  for (unsigned int gameNum = 0; gameNum < games.size(); ++gameNum) {
    const Game& g = games[gameNum];

    printf("Hand #%u\n", gameNum + 1);

    PrintHand("Best", g.playerCards, g.best);
    PrintHand("2nd", g.playerCards, g.secondBest);
    PrintHand("Worst", g.playerCards, g.worst);
  }
  fprintf(stderr, "\nPlay completed in %6.4f seconds.\n", (endTime - startTime).seconds());
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

  // For each game
  std::vector<Game>::iterator g;
  for (g = games.begin(); g != games.end(); ++g)
    PlayGame(*g);

  // Stop timing.
  tbb::tick_count endTime = tbb::tick_count::now();

  // Print the results.
  PrintGames(games, startTime, endTime);

  return 0;
}

