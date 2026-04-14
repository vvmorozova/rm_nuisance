#include <gtest/gtest.h>
#include "cmd_args_parcer.h"

// turns string list into char* argv[] for parse()
static std::vector<char*> make_argv(std::vector<std::string>& args) {
    std::vector<char*> argv;
    for (auto& s : args)
        argv.push_back(s.data());
    return argv;
}

// FR-4.1.1: no flags 
TEST(ArgsParserTest, SinglePositionalFile) {
    std::vector<std::string> args = {"rm_nuisance", "input.wav"};
    auto argv = make_argv(args);
    config cfg = cmd_args_parcer::parse((int)argv.size(), argv.data());

    ASSERT_EQ(cfg.input_files.size(), 1u);
    EXPECT_EQ(cfg.input_files[0], "input.wav");
    EXPECT_TRUE(cfg.output_file.empty());
    EXPECT_FALSE(cfg.verbose);
    EXPECT_FALSE(cfg.pack_mode);
}

// FR-4.1.2: -i & -o
TEST(ArgsParserTest, InputOutputFlags) {
    std::vector<std::string> args = {
        "rm_nuisance", "-i", "input.wav", "-o", "output.wav"
    };
    auto argv = make_argv(args);
    config cfg = cmd_args_parcer::parse((int)argv.size(), argv.data());

    ASSERT_EQ(cfg.input_files.size(), 1u);
    EXPECT_EQ(cfg.input_files[0], "input.wav");
    EXPECT_EQ(cfg.output_file, "output.wav");
}

// FR-4.1.3: --pack with several files 
TEST(ArgsParserTest, PackMode) {
    std::vector<std::string> args = {
        "rm_nuisance", "--pack", "a.wav", "b.wav", "c.wav"
    };
    auto argv = make_argv(args);
    config cfg = cmd_args_parcer::parse((int)argv.size(), argv.data());

    EXPECT_TRUE(cfg.pack_mode);
    ASSERT_EQ(cfg.input_files.size(), 3u);
    EXPECT_EQ(cfg.input_files[0], "a.wav");
    EXPECT_EQ(cfg.input_files[2], "c.wav");
}

// FR-4.2.1: --disable-type single
TEST(ArgsParserTest, DisableSingleType) {
    std::vector<std::string> args = {
        "rm_nuisance", "--disable-type", "noise", "input.wav"
    };
    auto argv = make_argv(args);
    config cfg = cmd_args_parcer::parse((int)argv.size(), argv.data());

    ASSERT_EQ(cfg.disabled_types.size(), 1u);
    EXPECT_EQ(cfg.disabled_types[0], nuisance_type::Noise);
}

// FR-4.2.1: --disable-type several 
TEST(ArgsParserTest, DisableMultipleTypes) {
    std::vector<std::string> args = {
        "rm_nuisance",
        "--disable-type", "filler",
        "--disable-type", "clicks",
        "--disable-type", "breath",
        "input.wav"
    };
    auto argv = make_argv(args);
    config cfg = cmd_args_parcer::parse((int)argv.size(), argv.data());

    ASSERT_EQ(cfg.disabled_types.size(), 3u);
    EXPECT_EQ(cfg.disabled_types[0], nuisance_type::Filler);
    EXPECT_EQ(cfg.disabled_types[1], nuisance_type::Clicks);
    EXPECT_EQ(cfg.disabled_types[2], nuisance_type::Breath);
}

// FR-4.2.2: --config
TEST(ArgsParserTest, ConfigFile) {
    std::vector<std::string> args = {
        "rm_nuisance", "--config", "path/to/config.json", "input.wav"
    };
    auto argv = make_argv(args);
    config cfg = cmd_args_parcer::parse((int)argv.size(), argv.data());

    EXPECT_EQ(cfg.config_file, "path/to/config.json");
}

// FR-4.2.3: --verbose 
TEST(ArgsParserTest, VerboseFlag) {
    std::vector<std::string> args = {"rm_nuisance", "--verbose", "input.wav"};
    auto argv = make_argv(args);
    config cfg = cmd_args_parcer::parse((int)argv.size(), argv.data());

    EXPECT_TRUE(cfg.verbose);
}

// several args
TEST(ArgsParserTest, CombinedOptions) {
    std::vector<std::string> args = {
        "rm_nuisance",
        "-i", "input.wav",
        "-o", "output.wav",
        "--disable-type", "pauses",
        "--config", "cfg.json",
        "--verbose"
    };
    auto argv = make_argv(args);
    config cfg = cmd_args_parcer::parse((int)argv.size(), argv.data());

    EXPECT_EQ(cfg.input_files[0], "input.wav");
    EXPECT_EQ(cfg.output_file,    "output.wav");
    EXPECT_EQ(cfg.config_file,    "cfg.json");
    EXPECT_TRUE(cfg.verbose);
    ASSERT_EQ(cfg.disabled_types.size(), 1u);
    EXPECT_EQ(cfg.disabled_types[0], nuisance_type::Pauses);
}

// error test cases
TEST(ArgsParserTest, NoArgsThrows) {
    std::vector<std::string> args = {"rm_nuisance"};
    auto argv = make_argv(args);
    EXPECT_THROW(cmd_args_parcer::parse((int)argv.size(), argv.data()),
                 std::invalid_argument);
}

TEST(ArgsParserTest, NoInputFileThrows) {
    std::vector<std::string> args = {"rm_nuisance", "--verbose"};
    auto argv = make_argv(args);
    EXPECT_THROW(cmd_args_parcer::parse((int)argv.size(), argv.data()),
                 std::invalid_argument);
}

TEST(ArgsParserTest, UnknownOptionThrows) {
    std::vector<std::string> args = {"rm_nuisance", "--unknown", "input.wav"};
    auto argv = make_argv(args);
    EXPECT_THROW(cmd_args_parcer::parse((int)argv.size(), argv.data()),
                 std::invalid_argument);
}

TEST(ArgsParserTest, UnknownNuisanceTypeThrows) {
    std::vector<std::string> args = {
        "rm_nuisance", "--disable-type", "aliens", "input.wav"
    };
    auto argv = make_argv(args);
    EXPECT_THROW(cmd_args_parcer::parse((int)argv.size(), argv.data()),
                 std::invalid_argument);
}

TEST(ArgsParserTest, MissingValueForOptionThrows) {
    std::vector<std::string> args = {"rm_nuisance", "-i"};
    auto argv = make_argv(args);
    EXPECT_THROW(cmd_args_parcer::parse((int)argv.size(), argv.data()),
                 std::invalid_argument);
}