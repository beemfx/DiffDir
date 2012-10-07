
#include <Windows.h>
#include <map>

class CDiffer
{
public:
	class IDifferCB;
	CDiffer(IDifferCB* pDiffer); //When scanning directories.
	CDiffer(IDifferCB* pDiffer, LPCTSTR strFile); //When reading the diff file.
	~CDiffer();

public:
	enum DIFF_T{SHOW, SAME, DATE, ADDED, DELETED};
	class IDifferCB
	{
	public: virtual void OnDiff(DIFF_T diff, LPCTSTR strFilename)=0;
	};
public:
	void SaveDiffInfo(LPCTSTR strFile);
	void SaveFileList(LPCTSTR strFile);
	void AddFile(LPCTSTR strFullPath, WIN32_FIND_DATA& FD);
	void ShowFiles();
	int  GetNumFiles()const;

	//Returns the number of diffs found
	DWORD CompareTo(const CDiffer& rhs);
	DWORD operator==(const CDiffer& rhs);
private:
	void LoadDiffInfo(LPCTSTR strFile);
	void LoadDiffInfo_ParseLine(LPCTSTR strLine);
private:
	struct SFileData
	{
	std::string strFilename;
	FILETIME    FileTime;
	};

	class CCmp
	{
		public: bool operator()(const std::string& s1, const std::string& s2)const
		{
			return s1.compare(s2) < 0;
		}
	};

	typedef std::map<std::string, SFileData, CCmp> CFileTree;

	CFileTree m_Files;

	IDifferCB* m_pOut;
};