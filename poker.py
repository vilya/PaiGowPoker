import itertools
import sys


def parse_hands(lines):
  hands = []
  for i in xrange(0, len(lines), 2):
    mine = tuple([lines[i][j:j+2] for j in xrange(0, len(lines[i]), 2)])
    theirs = tuple([lines[i+1][j:j+2] for j in xrange(0, len(lines[i+1]), 2)])
    hands.append( (mine, theirs) )
  return tuple(hands)


def all_hands(hand):
  for low_hand in itertools.combinations(hand, 2):
    remaining_cards = [x for x in hand if x not in low_hand]
    for high_hand in itertools.combinations(remaining_cards, 5):
      yield low_hand, high_hand


def card_value(card):
  return "0123456789XJQKA".index(card[0])


def hand_value(hand):
  '''hand_value(hand) -> int
  
  Returns a value for the hand such that for any two hands M and N, if
  hand_value(M) > hand_value(N) then hand M beats hand N and
  hand_value(M) == hand_value(N) means that the two hands draw with each other.

  This takes into account when the only difference between the hands is the
  face value of the cards (e.g. when both hands contain two pairs, but their
  cards have different face values).
  '''
  hand = sorted(hand, key=card_value, reverse=True)
  
  counts = []
  largest_group = 0
  i = 0
  while i < 5:
    val = card_value(hand[i])
    j = i + 1
    while j < 5 and val == card_value(hand[j]):
      j += 1
    count = j - i
    counts.append( (val, count) )
    if count > largest_group:
      largest_group = count
    i = j

  counts = sorted(counts, key=lambda x: x[1] * 15 + x[0], reverse=True)

  score = 0
  for val, count in counts:
    score *= 15
    score += val

  if largest_group == 1:
    is_straight = all([card_value(hand[i]) == card_value(hand[0]) - i for i in xrange(1,len(hand))])
    is_flush = all([card[1] == hand[0][1] for card in hand[1:]])

    if is_straight and is_flush:
      score += 8000000  # straight flush
    elif is_flush:
      score += 5000000  # flush
    elif is_straight:
      score += 4000000  # straight
    else:
      score += 0        # nothing
  elif len(counts) == 4:
    score += 1000000    # one pair
  elif len(counts) == 3:
    if largest_group == 3:
      score += 3000000  # three of a kind
    else:
      score += 2000000  # two pairs
  elif len(counts) == 2:
    if largest_group == 4:
      score += 7000000  # four of a kind
    else:
      score += 6000000  # full house
  elif len(counts) == 0:
    raise Exception("Cheat! You can't get 5 of a kind!")

  return score


def small_hand_value(hand):
  hand = sorted(hand, key=card_value, reverse=True)
  values = card_value(hand[0]), card_value(hand[1])
  score = values[0] * 15 + values[1]
  if values[0] == values[1]:
    score += 1000
  return score


def evaluate_hands(my_cards, their_cards):
  my_hands = tuple(all_hands(my_cards))
  my_scores = tuple([ (small_hand_value(low), hand_value(high)) for low, high in my_hands ])

  low_scores = sorted(list(set([low for low, _ in my_scores])), reverse=True)
  print "%2d low hand scores:" % len(low_scores), low_scores
  high_scores = sorted(list(set([high for _, high in my_scores])), reverse=True)
  print "%2d high hand scores:" % len(high_scores), high_scores
  for i in xrange(len(my_hands)):
    low_hand, high_hand = my_hands[i]
    low_score, high_score = my_scores[i]
    low_score_rank = low_scores.index(low_score)
    high_score_rank = high_scores.index(high_score)
    print "Hand #%2d:\t%s %s | %s %s %s %s %s\t\tScore: %4d (#%2d), %9d (#%2d)" % (i,
        low_hand[0], low_hand[1], high_hand[0], high_hand[1], high_hand[2], high_hand[3], high_hand[4],
        low_score, low_score_rank, high_score, high_score_rank)

  their_scores = tuple([ (small_hand_value(low), hand_value(high)) for low, high in all_hands(their_cards) ])

  wins = [0] * len(my_hands)
  draws = [0] * len(my_hands)
  losses = [0] * len(my_hands)

  for mine in xrange(len(my_hands)):
    for theirs in xrange(len(their_scores)):
      '''
      if my_scores[mine][0] == their_scores[theirs][0] or my_scores[mine][1] == their_scores[theirs][1]:
        draws[mine] += 1
        continue
      elif (my_scores[mine][0] > their_scores[theirs][0]) != (my_scores[mine][1] > their_scores[theirs][1]):
        draws[mine] += 1
      elif my_scores[mine][0] > their_scores[theirs][0]:
        wins[mine] += 1
      else:
        losses[mine] += 1
      '''
      cmpVal = 0
      for i in (0, 1):
        cmpVal += cmp(my_scores[mine][i], their_scores[theirs][i])
      
      if cmpVal < 0:
        losses[mine] += 1
      elif cmpVal > 1:
        wins[mine] += 1
      else:
        #print "DRAW: my_scores=", my_scores[mine], " their_scores=", their_scores[theirs]
        draws[mine] += 1

  results = sorted(zip(my_hands, zip(wins, draws, losses), range(len(my_hands))), key=lambda x: x[1], reverse=True)
  best = results[0]
  second_best = results[1]
  worst = results[-1]
  return best, second_best, worst


def print_hand(title, index, low_hand, high_hand, wins, draws, losses):
  print "%s:\t%s %s | %s %s %s %s %s\t\tW: %9d  D: %9d  L: %9d\t(Hand #%d)" % (title,
      low_hand[0], low_hand[1], high_hand[0], high_hand[1], high_hand[2],
      high_hand[3], high_hand[4], wins, draws, losses, index)


def main():
  if len(sys.argv) not in (1, 2):
    print >> sys.stderr, "Usage: %s [<infile>]" % sys.argv[0]
    sys.exit(-1)

  if len(sys.argv) == 2:
    f = open(sys.argv[1])
  else:
    f = sys.stdin

  lines = [line.strip() for line in f]

  if f != sys.stdin:
    f.close()

  i = 1
  for mine, theirs in parse_hands(lines):
    best, second_best, worst = evaluate_hands(mine, theirs)
    print "Hand #%d" % i
    hand, results, index = best
    print_hand("Best", index, hand[0], hand[1], *results)
    hand, results, index = second_best
    print_hand("2nd", index, hand[0], hand[1], *results)
    hand, results, index = worst
    print_hand("Worst", index, hand[0], hand[1], *results)
    i += 1
    


if __name__ == '__main__':
  main()

