static unsigned int GetVersionSourceRevisionNumber()
{
	static const unsigned int uSourceRevisionNumber = 4030;
	return uSourceRevisionNumber;
}

static unsigned int GetVersionBuildDate()
{
	static const unsigned int uBuildDate = 20131219;
	return uBuildDate;
}

static bool GetVersionIsExperimental()
{
	static const bool bExperimental = true;
	return bExperimental;
}

static bool GetVersionIsDebug()
{
#ifdef _DEBUG
	static const bool bDebug = true;
#else
	static const bool bDebug = false;
#endif

	return bDebug;
}
