import itertools
import random
import sys


def main():
  if len(sys.argv) != 2:
    print >> sys.stderr, "Usage: %s <num-dealer-cards>" % sys.argv[0]
    sys.exit(-1)

  numDealerCards = int(sys.argv[1])
  if numDealerCards > 45:
    raise Exception("Not enough cards")
  elif numDealerCards < 7:
    raise Exception("Must take at least 7 cards")

  cards = []
  for suit in "CDHS":
    for value in "23456789XJQKA":
      cards.append(value + suit)
  random.shuffle(cards)

  playerCards = cards[:7]
  cards = cards[7:]

  dealerCards = cards[:numDealerCards]

  print "".join(playerCards)
  print "".join(dealerCards)


  
if __name__ == '__main__':
  main()
