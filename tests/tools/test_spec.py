
#
# test_spec.py
#
# Unit tests of tools/spec.py, the specification checker. Run by CTest:
#	python3 -m unittest discover -s tests/tools
#

import os
import sys
import unittest

sys.path.insert(0, os.path.join(os.path.dirname(__file__), "..", "..",
	"tools"))

import spec  # noqa: E402

GOOD = """
# A comment.

story combat.duel
	title: Duels
	milestone: 0
	links: combat.strike
	description: Two troops fight.
	value distance: open distance
	criterion 1: unit
		then: A duel starts at {combat.duel.distance}.
		expect: duels.started = 1
	criterion 2: review proposed
		then: Something to decide.

story combat.strike
	title: Strikes
	milestone: later
	description: Troops strike.
"""


# References are built from parts, so this file holds none itself.
def reference(ids):
	return "//" + " spec: " + ids


def problems_of(text):
	return spec.read_document("spec.txt", text)[1]


class Document(unittest.TestCase):

	# spec: tools.check#1
	def test_a_good_document_has_no_problems(self):
		document, problems = spec.read_document("spec.txt", GOOD)
		self.assertEqual(problems, [])
		self.assertEqual(len(document.stories), 2)
		self.assertEqual(document.stories["combat.duel"].milestone, 0)

	# spec: tools.check#1
	def test_malformed_stories_are_reported(self):
		problems = problems_of("\nstory Combat\n\ttitle: x\n")
		self.assertEqual(problems[0].message, "malformed story line")

	# spec: tools.check#1
	def test_duplicate_ids_are_reported(self):
		text = GOOD + "\nstory combat.duel\n\ttitle: x\n\tmilestone: 0\n" \
			"\tdescription: x\n"
		messages = [problem.message for problem in problems_of(text)]
		self.assertIn("duplicate story", messages)

	# spec: tools.check#1
	def test_links_to_missing_stories_are_reported(self):
		text = GOOD.replace("links: combat.strike", "links: combat.parry")
		messages = [problem.message for problem in problems_of(text)]
		self.assertEqual(messages, ["link to no story"])

	# spec: tools.check#1
	def test_references_to_missing_values_are_reported(self):
		text = GOOD.replace("{combat.duel.distance}", "{combat.duel.range}")
		messages = [problem.message for problem in problems_of(text)]
		self.assertEqual(messages, ["reference to no value"])

	# spec: tools.check#1
	def test_malformed_lines_are_reported(self):
		for old, new in (
				("criterion 1: unit", "criterion 1: units"),
				("criterion 2: review proposed", "criterion 3: review"),
				("value distance: open distance", "value distance: open miles"),
				("expect: duels.started = 1", "expect: duels.started is 1"),
				("milestone: 0", "milestone: 9"),
				("\t\tthen: Something", "\t\tthan: Something")):
			with self.subTest(new=new):
				self.assertNotEqual(problems_of(GOOD.replace(old, new)), [])


class References(unittest.TestCase):

	def setUp(self):
		self.document = spec.read_document("spec.txt", GOOD)[0]

	# spec: tools.check#1
	def test_references_to_missing_ids_are_reported(self):
		text = "\n" + reference("combat.duel#9 combat.nothing") + "\n"
		problems = spec.check_references(self.document, "a.c", text, False)
		self.assertEqual([problem.message for problem in problems],
			["reference to no criterion", "reference to no story"])

	# spec: tools.check#1
	def test_agreed_criteria_without_tests_are_reported(self):
		problems = spec.check_coverage(self.document, 0)
		self.assertEqual([problem.detail for problem in problems],
			["combat.duel#1"])

	# spec: tools.check#1
	def test_tests_mark_criteria_as_tested(self):
		text = "\n" + reference("combat.duel#1") + "\n" \
			"cairo_test_new(duel, starts) {\n"
		problems = spec.check_references(self.document, "tests/a.c", text,
			True)
		self.assertEqual(problems, [])
		self.assertEqual(spec.check_coverage(self.document, 0), [])

	# spec: tools.check#1
	def test_fuzz_criteria_wait_for_the_fuzz_build(self):
		text = GOOD.replace("criterion 1: unit", "criterion 1: fuzz") + \
			"\nstory build.fuzz\n\ttitle: Fuzzing\n\tmilestone: 3\n" \
			"\tdescription: x\n"
		document = spec.read_document("spec.txt", text)[0]
		self.assertEqual(spec.check_coverage(document, 0), [])
		self.assertEqual(len(spec.check_coverage(document, 3)), 1)

	# spec: testing.unit#1
	def test_tests_must_name_criteria(self):
		text = "\nstatic int x;\ncairo_test_new(duel, starts) {\n"
		problems = spec.check_references(self.document, "tests/a.c", text,
			True)
		self.assertEqual([problem.message for problem in problems],
			["test names no criteria"])


class Format(unittest.TestCase):

	# spec: build.style#1
	def test_the_file_rules(self):
		self.assertEqual(spec.check_format("a", ""), [])
		self.assertEqual(spec.check_format("a", "\nfine\n"), [])
		for text in ("first\n", "\nno end", "\ntwo ends\n\n",
				"\ncr\r\n", "\ntrailing \n", "\n" + "x" * 81 + "\n",
				"\n" + "\t" * 20 + "x\n"):
			with self.subTest(text=text):
				self.assertNotEqual(spec.check_format("a", text), [])


if __name__ == "__main__":
	unittest.main()
