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
void PrintGames(const std::vector<Game>& games);


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
    printf("Dealer:");
    PrintHand(g->dealerCards);
    printf("\n");
    printf("----\n");
  }
}


void FirstCombination(unsigned int total, unsigned int numToChoose, unsigned int* combination)
{
  for (unsigned int i = 0; i < numToChoose; ++i)
    combination[i] = i;

  printf("%3u", combination[0]);
  for (unsigned int pos = 1; pos < numToChoose; ++pos)
    printf(" %3u", combination[pos]);
  printf("\n");
}


bool NextCombination(unsigned int total, unsigned int numToChoose, unsigned int* combination)
{
  const unsigned int kBaseMaxVal = total - numToChoose;

  int i = numToChoose - 1;
  while (i >= 0 && combination[i] == kBaseMaxVal + i)
    --i;

  if (i < 0)
    return false; // We've run out of combinations.

  ++combination[i];
  ++i;
  while (i < (int)numToChoose) {
    combination[i] = combination[i - 1] + 1;
    ++i;
  }

  printf("%3u", combination[0]);
  for (unsigned int pos = 1; pos < numToChoose; ++pos)
    printf(" %3u", combination[pos]);
  printf("\n");

  return true;
}


int main(int argc, char** argv)
{
  if (argc != 3) {
    fprintf(stderr, "Usage: %s <n> <k>\n", argv[0]);
    return -1;
  }
  unsigned int total = (unsigned int)atoi(argv[1]);
  unsigned int numToChoose = (unsigned int)atoi(argv[2]);

  unsigned int* combination = new unsigned int[numToChoose];

  FirstCombination(total, numToChoose, combination);
  while (NextCombination(total, numToChoose, combination))
    ;
  delete[] combination;


  /*
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
  */

  return 0;
}

