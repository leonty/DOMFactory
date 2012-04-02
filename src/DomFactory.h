#pragma once

#include <QtCore/QByteArray>
#include <QtCore/QMap>
#include <QtCore/QPair>
#include <QtCore/QCache>
#include <QtCore/QBuffer>

#include <QtXml/QDomDocument>
#include <QtXml/QXmlStreamReader>

namespace DomFactory
{

namespace
{
	/// Clean a path.
	QString& CleanPath(QString& aPath);
}

//------------------------------------------------------------------------------
/// Builds a QDomDocument from raw data.
class Builder
{
public:
	// Constructs an empty builder.
	Builder();

	// Constructs a builder and sets the data.
	Builder(QByteArray aData);

	/// Sets the buffer containing raw xml data.
	void setData(QByteArray aData);

	/// Sets the recent queries cache.
	void setCacheSize(uint aSize);

	/// Clears the cache.
	void clearCache();

	/// Searches for a tag using the given path and builds a document based on it.
	/// Returns the document cost (size of underlying raw xml data) or -1 if any error.
	int build(QString aPath, QDomDocument& aElement, uint aLeafTagIndex = 0);

	/// Searches for a tag using the given path and returns its raw data.
	bool find(QString aPath, QByteArray& aData, uint aLeafTagIndex = 0);

	/// Returns the error message in case if some operation finished unsuccessfully.
	QString lastErrorMessage() const;

private:
	// The copy constructor cannot copy the cache and the reader
	Builder(const Builder& aCopy);

	bool checkCache(QString aPath, qint64& aStartOffset, qint64& aEndOffset) const;
	void updateCache(QString aPath, qint64 aStartOffset, qint64 aEndOffset) const;

	typedef QPair<qint64, qint64> TCacheItem;
	typedef QCache<QString, TCacheItem> TCache;

	QBuffer mData;
	QXmlStreamReader mReader;
	mutable TCache mCache;
	QString mLastErrorMessage;
	QString mLastPath;
	uint mLastLeafTagIndex;
};

//------------------------------------------------------------------------------
/// Serves as a factory of dom documents managing with several buffers with raw xml data.
class Factory
{
public:
	~Factory();

	/// Appends a named raw xml data.
	void addData(QString aDomain, QByteArray aData);

	/// Removes data for the specified domain.
	bool deleteData(QString aDomain);

	/// Returns true if there's any data for the specified domain.
	bool hasDomain(QString aDomain) const;

	/// Sets the recent dom documents cache.
	void setCacheSize(QString aDomain, size_t aSize);

	/// Clears all caches.
	void clearCache();

	/// Clears the spicified domain cache.
	void clearCache(QString aDomain);

	/// Searches for a tag in the specefied domain using the given path and builds a document based on it.
	bool build(QString aDomain, QString aPath, QDomDocument &aDocument, uint aLeafTagIndex = 0) const;

	/// Searches for a tag in the specefied domain using the given path and returns its raw data.
	bool find(QString aDomain, QString aPath, QByteArray &aData, uint aLeafTagIndex = 0) const;

	/// Returns the error message in case if an operation for the specified domain finished unsuccessfully.
	QString lastErrorMessage(QString aDomain) const;

	/// Returns the error message in case if an operation for the factory finished unsuccessfully.
	QString lastErrorMessage() const;

private:
	bool checkCache(QString aDomain, QString aPath, QDomDocument &aDocument) const;

	typedef QCache<QString, QDomDocument> TDomDocumentCache;
	typedef QPair<Builder*, TDomDocumentCache*> TBuilderCachePair;
	typedef QMap<QString, TBuilderCachePair> TBuilderMap;

	TBuilderMap mBuilders;
	mutable QString mLastErrorMessage;
};

} // namespade DomFactory

