/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2019, Google Inc.
 *
 * options.h - cam - Options parsing
 */
#ifndef __CAM_OPTIONS_H__
#define __CAM_OPTIONS_H__

#include <ctype.h>
#include <list>
#include <map>
#include <vector>

class KeyValueParser;
class OptionValue;
struct Option;

enum OptionArgument {
	ArgumentNone,
	ArgumentRequired,
	ArgumentOptional,
};

enum OptionType {
	OptionNone,
	OptionInteger,
	OptionString,
	OptionKeyValue,
};

template<typename T>
class OptionsBase
{
public:
	OptionsBase() : valid_(false) {}

	bool empty() const;
	bool valid() const;
	bool isSet(const T &opt) const;
	const OptionValue &operator[](const T &opt) const;

	void invalidate();

private:
	friend class KeyValueParser;
	friend class OptionsParser;

	bool parseValue(const T &opt, const Option &option, const char *value);

	std::map<T, OptionValue> values_;
	bool valid_;
};

class KeyValueParser
{
public:
	class Options : public OptionsBase<std::string>
	{
	};

	KeyValueParser();
	virtual ~KeyValueParser();

	bool addOption(const char *name, OptionType type, const char *help,
		       OptionArgument argument = ArgumentNone);

	virtual Options parse(const char *arguments);

private:
	friend class OptionsParser;
	void usage(int indent);

	std::map<std::string, Option> optionsMap_;
};

class OptionsParser
{
public:
	class Options : public OptionsBase<int>
	{
	};

	OptionsParser();
	~OptionsParser();

	bool addOption(int opt, OptionType type, const char *help,
		       const char *name = nullptr,
		       OptionArgument argument = ArgumentNone,
		       const char *argumentName = nullptr, bool array = false);
	bool addOption(int opt, KeyValueParser *parser, const char *help,
		       const char *name = nullptr, bool array = false);

	Options parse(int argc, char *argv[]);
	void usage();

private:
	void parseValueError(const Option &option);

	std::list<Option> options_;
	std::map<unsigned int, Option *> optionsMap_;
};

class OptionValue
{
public:
	enum ValueType {
		ValueNone,
		ValueInteger,
		ValueString,
		ValueKeyValue,
		ValueArray,
	};

	OptionValue();
	OptionValue(int value);
	OptionValue(const char *value);
	OptionValue(const std::string &value);
	OptionValue(const KeyValueParser::Options &value);

	void addValue(const OptionValue &value);

	ValueType type() const { return type_; }

	operator int() const;
	operator std::string() const;
	operator KeyValueParser::Options() const;
	operator std::vector<OptionValue>() const;

	int toInteger() const;
	std::string toString() const;
	KeyValueParser::Options toKeyValues() const;
	std::vector<OptionValue> toArray() const;

private:
	ValueType type_;
	int integer_;
	std::string string_;
	KeyValueParser::Options keyValues_;
	std::vector<OptionValue> array_;
};

#endif /* __CAM_OPTIONS_H__ */
