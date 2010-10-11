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
  bool jumpTo(unsigned int n);

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

  //unsigned int* combination;

  PlayerHand() : index(0), lowHandScore(0), highHandScore(0), wins(0), draws(0), losses(0)
  //   , combination(NULL)
  {}
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


struct GameReduce
{
  Game _game;
  PlayerHand _playerHands[21];

  GameReduce(const Game& game);
  GameReduce(GameReduce& other, tbb::split);

  void operator () (const tbb::blocked_range<unsigned int>& range);
  void join(GameReduce& rhs);

  void finish();
};


//
// Forward declarations
//

unsigned int Choose(unsigned int n, unsigned int k);

Card ParseCard(const char* card);
std::vector<Card> ParseHand(char* line);
std::vector<Game> ParseGames(FILE* f);

bool CompareCardsDescending(const Card& a, const Card& b);
bool ComparePlayerHandsDescending(const PlayerHand& a, const PlayerHand& b);

unsigned int ScoreLowHand(const Card cards[], const Combinations& combo);
unsigned int ScoreHighHand(const Card cards[], const Combinations& combo);

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


bool Combinations::jumpTo(unsigned int i)
{
  const unsigned int kNumCombos = Choose(_kTotal, _kNum);
  if (i >= kNumCombos)
    return false;

  unsigned int a = _kTotal;
  unsigned int b = _kNum;
  unsigned int x = kNumCombos - i - 1;

  for (unsigned int index = 0; index < _kNum; ++index) {
    unsigned int v = a;
    while (Choose(v, b) > x)
      --v;
    _combination[index] = v;
    x -= Choose(v, b);
    a = v;
    --b;
  }

  for (unsigned int index = 0; index < _kNum; ++index)
    _combination[index] = (_kTotal - 1) - _combination[index];

  return true;
}


const unsigned int* Combinations::current() const
{
  return _combination;
}


//
// GameReduce methods
//

GameReduce::GameReduce(const Game& game) :
  _game(game)
{
  // We assume that the player always has 7 cards.

  // Calculate the scores for all possible player hands.
  Combinations playerCombos(7, 2);
  unsigned int i = 0;
  do {
    _playerHands[i].index = i;
    _playerHands[i].lowHandScore = ScoreLowHand(game.playerCards.data(), playerCombos);
    _playerHands[i].highHandScore = ScoreHighHand(game.playerCards.data(), playerCombos);
    _playerHands[i].wins = 0;
    _playerHands[i].draws = 0;
    _playerHands[i].losses = 0;

    /*
    _playerHands[i].combination = new unsigned int[2];
    _playerHands[i].combination[0] = playerCombos.current()[0];
    _playerHands[i].combination[1] = playerCombos.current()[1];
    */

    ++i;
  } while (playerCombos.next());
}


GameReduce::GameReduce(GameReduce& other, tbb::split) :
  _game(other._game)
{
  for (unsigned int i = 0; i < 21; ++i) {
    _playerHands[i] = other._playerHands[i];
    _playerHands[i].wins = 0;
    _playerHands[i].draws = 0;
    _playerHands[i].losses = 0;
  }
}


void GameReduce::operator () (const tbb::blocked_range<unsigned int>& range)
{
  const Game& game = _game;

  // For each possible dealer hand, play each possible player hand against it and record the results.
  Card tmpDealerHand[7];
  Combinations dealerCombos(game.dealerCards.size(), 7);

  dealerCombos.jumpTo(range.begin());
  for (unsigned int index = range.begin(); index < range.end(); ++index) {
    for (unsigned int i = 0; i < 7; ++i)
      tmpDealerHand[i] = game.dealerCards.data()[dealerCombos.current()[i]];
    
    Combinations dealerHandCombos(7, 2);
    do {
      unsigned int dealerLowScore = ScoreLowHand(tmpDealerHand, dealerHandCombos);
      unsigned int dealerHighScore = ScoreHighHand(tmpDealerHand, dealerHandCombos);
      for (unsigned int i = 0; i < 21; ++i) {
        unsigned int playerLowScore = _playerHands[i].lowHandScore;
        unsigned int playerHighScore = _playerHands[i].highHandScore;
        if (playerLowScore > dealerLowScore && playerHighScore > dealerHighScore)
          ++_playerHands[i].wins;
        else if (playerLowScore < dealerLowScore && playerHighScore < dealerHighScore)
          ++_playerHands[i].losses;
        else
          ++_playerHands[i].draws;
      }
    } while (dealerHandCombos.next());

    dealerCombos.next();
  }
}


void GameReduce::join(GameReduce& rhs)
{
  for (unsigned int i = 0; i < 21; ++i) {
    _playerHands[i].wins += rhs._playerHands[i].wins;
    _playerHands[i].draws += rhs._playerHands[i].draws;
    _playerHands[i].losses += rhs._playerHands[i].losses;
  }
}


void GameReduce::finish()
{
  // Find the player hands with the best, second best and worst results.
  std::sort(_playerHands, _playerHands + 21, ComparePlayerHandsDescending);
  _game.best = _playerHands[0];
  _game.secondBest = _playerHands[1];
  _game.worst = _playerHands[20];

  // Clean up the memory for the playerHand combinations.
//  for (unsigned int i = 0; i < 21; ++i)
//    delete[] _playerHands[i].combination;
}


//
// Functions
//

unsigned int Choose(unsigned int n, unsigned int k)
{
  size_t top = 1;
  for (size_t val = n - k + 1; val <= n; ++val)
    top *= val;

  size_t bottom = 1;
  for (size_t val = 2; val <= k; ++val)
    bottom *= val;

  return (unsigned int)(top / bottom);
}


Card ParseCard(const char* card)
{
  Card c;
  switch (card[0]) {
    case '2': c.value = (1 << 0); break;
    case '3': c.value = (1 << 1); break;
    case '4': c.value = (1 << 2); break;
    case '5': c.value = (1 << 3); break;
    case '6': c.value = (1 << 4); break;
    case '7': c.value = (1 << 5); break;
    case '8': c.value = (1 << 6); break;
    case '9': c.value = (1 << 7); break;
    case 'X': c.value = (1 << 8); break;
    case 'J': c.value = (1 << 9); break;
    case 'Q': c.value = (1 << 10); break;
    case 'K': c.value = (1 << 11); break;
    case 'A': c.value = (1 << 12); break;
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
  return (a.value > b.value) || (a.value == b.value && a.suit > b.suit);
}


bool ComparePlayerHandsDescending(const PlayerHand& a, const PlayerHand& b)
{
  return (a.wins > b.wins) || (a.wins == b.wins && a.draws > b.draws);
}


unsigned int ScoreLowHand(const Card cards[], const Combinations& combo)
{
  const Card& a = cards[combo.current()[0]];
  const Card& b = cards[combo.current()[1]];

  unsigned int score = a.value | b.value;
  if (a.value == b.value)
    score |= (1 << 13);
  return score;
}


unsigned int ScoreHighHand(const Card cards[], const Combinations& combo)
{
  // This function assumes that there are 7 cards in the cards vector and that
  // the combo describes the low hand, i.e. the two cards which AREN'T in this
  // hand.

  // Get the current hand.
  unsigned int suits[4] = { 0, 0, 0, 0 };
  for (unsigned int i = 0; i < 7; ++i) {
    if (i != combo.current()[0] && i != combo.current()[1])
      suits[cards[i].suit] |= cards[i].value;
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
      return 1000000 + (__builtin_ctz(pairs) << 13) + groups; // One pair.
    }

    case 3:
    {
      unsigned int pairs = (suits[0] & suits[1]) | (suits[0] & suits[2]) | (suits[0] & suits[3])
        | (suits[1] & suits[2]) | (suits[1] & suits[3]) | (suits[2] & suits[3]);
      groups &= ~pairs;
      if (__builtin_popcount(pairs) > 1) // Two pairs.
        return 2000000 + ((32 - __builtin_clz(pairs)) << 8) + (__builtin_ctz(pairs) << 4) + __builtin_ctz(groups);
      else // Three of a kind
        return 3000000 + (__builtin_ctz(pairs) << 13) + groups;
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


void PrintCard(const Card& card)
{
  const char* values = "23456789XJQKA";
  const char* suits = "CDHS";

  printf("%c%c", values[__builtin_ctz(card.value)], suits[card.suit]);
}


void PrintHand(const char* title, const std::vector<Card>& cards, const PlayerHand& hand)
{
  Combinations combo(cards.size(), 2);
  combo.jumpTo(hand.index);

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
  for (g = games.begin(); g != games.end(); ++g) {
    const unsigned int kNumDealerCombos = Choose(g->dealerCards.size(), 7);
    GameReduce gameReduce(*g);
    tbb::parallel_reduce(tbb::blocked_range<unsigned int>(0, kNumDealerCombos), gameReduce);
    gameReduce.finish();
    *g = gameReduce._game;
  }

  // Stop timing.
  tbb::tick_count endTime = tbb::tick_count::now();

  // Print the results.
  PrintGames(games, startTime, endTime);

  return 0;
}

