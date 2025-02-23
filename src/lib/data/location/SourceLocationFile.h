#pragma once
// STL
#include <functional>
#include <map>
#include <memory>
#include <ostream>
#include <set>
// internal
#include "FilePath.h"
#include "LocationType.h"
#include "SourceLocation.h"
#include "types.h"

class SourceLocationFile final {
public:
  struct LocationComp final {
    bool operator()(const std::shared_ptr<SourceLocation>& lhs,
                    const std::shared_ptr<SourceLocation>& rhs) const {
      return *(lhs) < *(rhs);
    }
  };

  SourceLocationFile(
      FilePath filePath, std::wstring language, bool isWhole, bool isComplete, bool isIndexed);

  ~SourceLocationFile();

  const FilePath& getFilePath() const;

  void setLanguage(const std::wstring& language);

  const std::wstring& getLanguage() const;

  void setIsWhole(bool isWhole);

  bool isWhole() const;

  void setIsComplete(bool isComplete);

  bool isComplete() const;

  void setIsIndexed(bool isIndexed);

  bool isIndexed() const;

  const std::multiset<std::shared_ptr<SourceLocation>, LocationComp>& getSourceLocations() const;

  size_t getSourceLocationCount() const;

  size_t getUnscopedStartLocationCount() const;

  SourceLocation* addSourceLocation(LocationType type,
                                    Id locationId,
                                    std::vector<Id> tokenIds,
                                    size_t startLineNumber,
                                    size_t startColumnNumber,
                                    size_t endLineNumber,
                                    size_t endColumnNumber);

  SourceLocation* addSourceLocationCopy(const SourceLocation* location);

  void copySourceLocations(const std::shared_ptr<SourceLocationFile>& pFile);

  SourceLocation* getSourceLocationById(Id locationId) const;

  void forEachSourceLocation(const std::function<void(SourceLocation*)>& func) const;

  void forEachStartSourceLocation(const std::function<void(SourceLocation*)>& func) const;

  void forEachEndSourceLocation(const std::function<void(SourceLocation*)>& func) const;

  std::shared_ptr<SourceLocationFile> getFilteredByLines(size_t firstLineNumber,
                                                         size_t lastLineNumber) const;

  std::shared_ptr<SourceLocationFile> getFilteredByType(LocationType type) const;

  std::shared_ptr<SourceLocationFile> getFilteredByTypes(const std::vector<LocationType>& types) const;

private:
  const FilePath m_filePath;
  std::wstring m_language;
  bool m_isWhole;
  bool m_isComplete;
  bool m_isIndexed;

  std::multiset<std::shared_ptr<SourceLocation>, LocationComp> m_locations;
  std::map<Id, SourceLocation*> m_locationIndex;
};

std::wostream& operator<<(std::wostream& ostream, const SourceLocationFile& file);