#include <gtest/gtest.h>
#include "cmd_args_parcer.h"
#include <filesystem>
#include <fstream>

// turns string list into char* argv[] for parse()
static std::vector<char*> make_argv(std::vector<std::string>& args) {
    std::vector<char*> argv;
    for (auto& s : args)
        argv.push_back(s.data());
    return argv;
}

// helper to create a temporary test file
static std::string create_temp_file(const std::string& name) {
    std::string path = "/tmp/" + name;
    std::ofstream file(path);
    file << "test data";
    file.close();
    return path;
}

// FR-4.1.1: no flags
TEST(ArgsParserTest, SinglePositionalFile) {
    auto input = create_temp_file("test_input.wav");
    std::vector<std::string> args = {"rm_nuisance", input};
    auto argv = make_argv(args);
    config cfg = cmd_args_parcer::parse((int)argv.size(), argv.data());

    ASSERT_EQ(cfg.input_files.size(), 1u);
    EXPECT_EQ(cfg.input_files[0], input);
    EXPECT_TRUE(cfg.output_files.empty());

    std::filesystem::remove(input);
}

// FR-4.1.2: -i & -o
TEST(ArgsParserTest, InputOutputFlags) {
    auto input = create_temp_file("test_input2.wav");
    std::vector<std::string> args = {
        "rm_nuisance", "-i", input, "-o", "/tmp/output.wav"
    };
    auto argv = make_argv(args);
    config cfg = cmd_args_parcer::parse((int)argv.size(), argv.data());

    ASSERT_EQ(cfg.input_files.size(), 1u);
    EXPECT_EQ(cfg.input_files[0], input);
    EXPECT_EQ(cfg.output_files[0], "/tmp/output.wav");

    std::filesystem::remove(input);
}

// FR-4.2.1: --disable-type single
TEST(ArgsParserTest, DisableSingleType) {
    auto input = create_temp_file("test_input3.wav");
    std::vector<std::string> args = {
        "rm_nuisance", "--disable-type", "noise", input
    };
    auto argv = make_argv(args);
    config cfg = cmd_args_parcer::parse((int)argv.size(), argv.data());

    ASSERT_EQ(cfg.disabled_types.size(), 1u);
    EXPECT_EQ(cfg.disabled_types[0], nuisance_type::noise);

    std::filesystem::remove(input);
}

// FR-4.2.1: --disable-type several (boost collects last multitoken, so repeat the flag)
TEST(ArgsParserTest, DisableMultipleTypes) {
    auto input = create_temp_file("test_input4.wav");
    std::vector<std::string> args = {
        "rm_nuisance",
        "--disable-type", "filler",
        "--disable-type", "clicks",
        "--disable-type", "breath",
        input
    };
    auto argv = make_argv(args);
    config cfg = cmd_args_parcer::parse((int)argv.size(), argv.data());

    ASSERT_EQ(cfg.disabled_types.size(), 3u);
    EXPECT_EQ(cfg.disabled_types[0], nuisance_type::filler);
    EXPECT_EQ(cfg.disabled_types[1], nuisance_type::clicks);
    EXPECT_EQ(cfg.disabled_types[2], nuisance_type::breath);

    std::filesystem::remove(input);
}

// FR-4.2.2: model path specification
TEST(ArgsParserTest, ModelPathOption) {
    auto input = create_temp_file("test_input5.wav");
    std::vector<std::string> args = {
        "rm_nuisance", "-m", "/path/to/model.bin", input
    };
    auto argv = make_argv(args);
    config cfg = cmd_args_parcer::parse((int)argv.size(), argv.data());

    EXPECT_EQ(cfg.model_path, "/path/to/model.bin");

    std::filesystem::remove(input);
}

// FR-4.2.3: combined options
TEST(ArgsParserTest, CombinedOptions) {
    auto input = create_temp_file("test_input6.wav");
    std::vector<std::string> args = {
        "rm_nuisance",
        "-i", input,
        "-o", "/tmp/output.wav",
        "-m", "/path/to/model.bin",
        "--disable-type", "pauses"
    };
    auto argv = make_argv(args);
    config cfg = cmd_args_parcer::parse((int)argv.size(), argv.data());

    EXPECT_EQ(cfg.input_files[0], input);
    EXPECT_EQ(cfg.output_files[0], "/tmp/output.wav");
    EXPECT_EQ(cfg.model_path, "/path/to/model.bin");
    ASSERT_EQ(cfg.disabled_types.size(), 1u);
    EXPECT_EQ(cfg.disabled_types[0], nuisance_type::pauses);

    std::filesystem::remove(input);
}

// FR-4.2.4: multiple input files with -i and matching outputs
TEST(ArgsParserTest, MultipleInputFilesWithFlag) {
    auto input1 = create_temp_file("test_input7a.wav");
    auto input2 = create_temp_file("test_input7b.wav");
    std::vector<std::string> args = {
        "rm_nuisance",
        "-i", input1, input2,
        "-o", "/tmp/out1.wav", "/tmp/out2.wav"
    };
    auto argv = make_argv(args);
    config cfg = cmd_args_parcer::parse((int)argv.size(), argv.data());

    ASSERT_EQ(cfg.input_files.size(), 2u);
    EXPECT_EQ(cfg.input_files[0], input1);
    EXPECT_EQ(cfg.input_files[1], input2);
    ASSERT_EQ(cfg.output_files.size(), 2u);

    std::filesystem::remove(input1);
    std::filesystem::remove(input2);
}

// FR-4.3.1: --help flag
TEST(ArgsParserTest, HelpFlag) {
    std::vector<std::string> args = {"rm_nuisance", "--help"};
    auto argv = make_argv(args);
    config cfg = cmd_args_parcer::parse((int)argv.size(), argv.data());

    EXPECT_TRUE(cfg.call_help);
}

// FR-4.3.2: -h flag
TEST(ArgsParserTest, HelpShortFlag) {
    std::vector<std::string> args = {"rm_nuisance", "-h"};
    auto argv = make_argv(args);
    config cfg = cmd_args_parcer::parse((int)argv.size(), argv.data());

    EXPECT_TRUE(cfg.call_help);
}

// FR-4.3.3: help with input file
TEST(ArgsParserTest, HelpWithInput) {
    auto input = create_temp_file("test_input8.wav");
    std::vector<std::string> args = {"rm_nuisance", "-h", input};
    auto argv = make_argv(args);
    config cfg = cmd_args_parcer::parse((int)argv.size(), argv.data());

    EXPECT_TRUE(cfg.call_help);
    EXPECT_EQ(cfg.input_files[0], input);

    std::filesystem::remove(input);
}

// error cases

TEST(ArgsParserTest, NoArgsThrows) {
    std::vector<std::string> args = {"rm_nuisance"};
    auto argv = make_argv(args);
    EXPECT_THROW(cmd_args_parcer::parse((int)argv.size(), argv.data()),
                 std::invalid_argument);
}

TEST(ArgsParserTest, UnknownNuisanceTypeThrows) {
    auto input = create_temp_file("test_input9.wav");
    std::vector<std::string> args = {
        "rm_nuisance", "--disable-type", "aliens", input
    };
    auto argv = make_argv(args);
    EXPECT_THROW(cmd_args_parcer::parse((int)argv.size(), argv.data()),
                 std::invalid_argument);

    std::filesystem::remove(input);
}

// boost throws po::invalid_command_line_syntax when -m gets another flag as value
TEST(ArgsParserTest, MissingValueForOptionThrows) {
    auto input = create_temp_file("test_input10.wav");
    std::vector<std::string> args = {"rm_nuisance", "-m", "-i", input};
    auto argv = make_argv(args);
    EXPECT_THROW(cmd_args_parcer::parse((int)argv.size(), argv.data()),
                 std::exception);

    std::filesystem::remove(input);
}

TEST(ArgsParserTest, MissingInputFileThrows) {
    std::vector<std::string> args = {
        "rm_nuisance", "-i", "/nonexistent/file.wav"
    };
    auto argv = make_argv(args);
    EXPECT_THROW(cmd_args_parcer::parse((int)argv.size(), argv.data()),
                 std::runtime_error);
}

TEST(ArgsParserTest, AllNuisanceTypes) {
    auto input = create_temp_file("test_input11.wav");
    std::vector<std::string> args = {
        "rm_nuisance",
        "--disable-type", "filler",
        "--disable-type", "noise",
        "--disable-type", "clicks",
        "--disable-type", "pauses",
        "--disable-type", "breath",
        input
    };
    auto argv = make_argv(args);
    config cfg = cmd_args_parcer::parse((int)argv.size(), argv.data());

    ASSERT_EQ(cfg.disabled_types.size(), 5u);

    std::filesystem::remove(input);
}

TEST(ArgsParserTest, InputOutputCountMismatchThrows) {
    auto input1 = create_temp_file("test_input12a.wav");
    auto input2 = create_temp_file("test_input12b.wav");
    std::vector<std::string> args = {
        "rm_nuisance",
        "-i", input1, input2,
        "-o", "/tmp/only_one_output.wav"
    };
    auto argv = make_argv(args);
    EXPECT_THROW(cmd_args_parcer::parse((int)argv.size(), argv.data()),
                 std::invalid_argument);

    std::filesystem::remove(input1);
    std::filesystem::remove(input2);
}