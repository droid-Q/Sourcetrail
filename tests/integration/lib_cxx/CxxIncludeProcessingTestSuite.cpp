#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "IncludeDirective.h"
#include "IncludeProcessing.h"
#include "TextAccess.h"
#include "language_packages.h"
#include "utility.h"

TEST(CxxIncludeProcessing, includeDetectionFindsIncludeWithQuotes) {
  auto includeDirectives = IncludeProcessing::getIncludeDirectives(
      TextAccess::createFromString("#include \"foo.h\"", FilePath(L"foo.cpp")));

  ASSERT_FALSE(includeDirectives.empty());
  EXPECT_THAT(includeDirectives.front().getIncludedFile().wstr(), testing::StrEq(L"foo.h"));
  EXPECT_THAT(includeDirectives.front().getIncludingFile().wstr(), testing::StrEq(L"foo.cpp"));
}

TEST(CxxIncludeProcessing, includeDetectionFindsIncludeWithAngleBrackets) {
  auto includeDirectives = IncludeProcessing::getIncludeDirectives(
      TextAccess::createFromString("#include <foo.h>", FilePath(L"foo.cpp")));

  ASSERT_FALSE(includeDirectives.empty());
  EXPECT_THAT(includeDirectives.front().getIncludedFile().wstr(), testing::StrEq(L"foo.h"));
  EXPECT_THAT(includeDirectives.front().getIncludingFile().wstr(), testing::StrEq(L"foo.cpp"));
}

TEST(CxxIncludeProcessing, includeDetectionFindsIncludeWithQuotesAndSpaceBeforeKeyword) {
  auto includeDirectives = IncludeProcessing::getIncludeDirectives(
      TextAccess::createFromString("# include \"foo.h\"", FilePath(L"foo.cpp")));

  ASSERT_FALSE(includeDirectives.empty());
  EXPECT_THAT(includeDirectives.front().getIncludedFile().wstr(), testing::StrEq(L"foo.h"));
  EXPECT_THAT(includeDirectives.front().getIncludingFile().wstr(), testing::StrEq(L"foo.cpp"));
}

TEST(CxxIncludeProcessing, includeDetectionDoesNotFindIncludeInEmptyFile) {
  EXPECT_TRUE(IncludeProcessing::getIncludeDirectives(TextAccess::createFromString("")).empty());
}

TEST(CxxIncludeProcessing, includeDetectionDoesNotFindIncludeInFileWithoutPreprocessorDirective) {
  EXPECT_TRUE(IncludeProcessing::getIncludeDirectives(TextAccess::createFromString("foo")).empty());
}

TEST(CxxIncludeProcessing, includeDetectionDoesNotFindIncludeInFileWithoutIncludePreprocessorDirective) {
  EXPECT_TRUE(IncludeProcessing::getIncludeDirectives(TextAccess::createFromString("#ifdef xx\n#endif")).empty());
}

TEST(CxxIncludeProcessing, headerSearchPathDetectionDoesNotFindPathRelativeToIncludingFile) {
  auto headerSearchDirectories = utility::toVector(IncludeProcessing::getHeaderSearchDirectories(
      {FilePath(L"data/CxxIncludeProcessingTestSuite/"
                L"test_header_search_path_detection_does_not_find_path_relative_to_including_"
                L"file/a.cpp")},
      {FilePath(L"data/CxxIncludeProcessingTestSuite/"
                L"test_header_search_path_detection_does_not_find_path_relative_to_including_"
                L"file")},
      {},
      1,
      [](float) {}));

  EXPECT_TRUE(headerSearchDirectories.empty());
}

TEST(CxxIncludeProcessing, headerSearchPathDetectionFindsPathInsideSubDirectory) {
  auto headerSearchDirectories = utility::toVector(IncludeProcessing::getHeaderSearchDirectories(
      {FilePath(L"data/CxxIncludeProcessingTestSuite/"
                L"test_header_search_path_detection_finds_path_inside_sub_directory/a.cpp")},
      {FilePath(L"data/CxxIncludeProcessingTestSuite/"
                L"test_header_search_path_detection_finds_path_inside_sub_directory")},
      {},
      1,
      [](float) {}));

  EXPECT_TRUE(
      utility::containsElement<FilePath>(headerSearchDirectories,
                                         FilePath(L"data/CxxIncludeProcessingTestSuite/"
                                                  L"test_header_search_path_detection_finds_path_inside_sub_directory/include")
                                             .makeAbsolute()));
}

TEST(CxxIncludeProcessing, headerSearchPathDetectionFindsPathRelativeToSubDirectory) {
  auto headerSearchDirectories = utility::toVector(IncludeProcessing::getHeaderSearchDirectories(
      {FilePath(L"data/CxxIncludeProcessingTestSuite/"
                L"test_header_search_path_detection_finds_path_relative_to_sub_directory/"
                L"a.cpp")},
      {FilePath(L"data/CxxIncludeProcessingTestSuite/"
                L"test_header_search_path_detection_finds_path_relative_to_sub_directory")},
      {},
      1,
      [](float) {}));

  EXPECT_TRUE(utility::containsElement<FilePath>(
      headerSearchDirectories,
      FilePath(L"data/CxxIncludeProcessingTestSuite/"
               L"test_header_search_path_detection_finds_path_relative_to_sub_directory/include")
          .makeAbsolute()));
}

TEST(CxxIncludeProcessing, headerSearchPathDetectionFindsPathIncludedInHeaderSearchPath) {
  auto headerSearchDirectories = utility::toVector(IncludeProcessing::getHeaderSearchDirectories(
      {FilePath(L"data/CxxIncludeProcessingTestSuite/"
                L"test_header_search_path_detection_finds_path_included_in_header_search_"
                L"path/a.cpp")},
      {FilePath(L"data/CxxIncludeProcessingTestSuite/"
                L"test_header_search_path_detection_finds_path_included_in_header_search_"
                L"path/include_b")},
      {FilePath(L"data/CxxIncludeProcessingTestSuite/"
                L"test_header_search_path_detection_finds_path_included_in_header_search_"
                L"path/include_a")},
      1,
      [](float) {}));


  EXPECT_TRUE(
      utility::containsElement<FilePath>(headerSearchDirectories,
                                         FilePath(L"data/CxxIncludeProcessingTestSuite/"
                                                  L"test_header_search_path_detection_finds_path_included_in_header_search_path/"
                                                  L"include_b")
                                             .makeAbsolute()));
}

TEST(CxxIncludeProcessing, headerSearchPathDetectionFindsPathIncludedInFutureHeaderSearchPath) {
  auto headerSearchDirectories = utility::toVector(IncludeProcessing::getHeaderSearchDirectories(
      {FilePath(L"data/CxxIncludeProcessingTestSuite/"
                L"test_header_search_path_detection_finds_path_included_in_future_header_"
                L"search_path/a.cpp")},
      {FilePath(L"data/CxxIncludeProcessingTestSuite/"
                L"test_header_search_path_detection_finds_path_included_in_future_header_"
                L"search_path")},
      {},
      1,
      [](float) {}));

  EXPECT_TRUE(utility::containsElement<FilePath>(
      headerSearchDirectories,
      FilePath(L"data/CxxIncludeProcessingTestSuite/"
               L"test_header_search_path_detection_finds_path_included_in_future_header_search_"
               L"path/include_a")
          .makeAbsolute()));
  EXPECT_TRUE(utility::containsElement<FilePath>(
      headerSearchDirectories,
      FilePath(L"data/CxxIncludeProcessingTestSuite/"
               L"test_header_search_path_detection_finds_path_included_in_future_header_search_"
               L"path/include_b")
          .makeAbsolute()));
}