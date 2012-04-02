#include <QtCore/QIODevice>
#include <QtCore/QStringList>

#include "DomFactory.h"

namespace DomFactory
{

namespace
{
/// Default cache sizes.
const uint cDefaultBuilderCacheSize = 50;
const uint cDefaultFactoryCacheSize = 5000;

// Error messages
const char cStartElementNotFound[] = "DomFactory::Builder: start element not found";
const char cEndElementNotFound[]   = "DomFactory::Builder: end element not found";
const char cNoDomain[]             = "DomFactory::Factory: no such domain ";

QString& CleanPath(QString& aPath)
{
	if (aPath.startsWith('/'))
	{
		aPath.remove(0, 1);
	}

	if (aPath.endsWith('/'))
	{
		aPath.remove(aPath.size()-1, 1);
	}

	return aPath;
}

} // private namespace

//------------------------------------------------------------------------------
Builder::Builder(): mLastLeafTagIndex(0)
{
	setCacheSize(cDefaultBuilderCacheSize);
}

//------------------------------------------------------------------------------
Builder::Builder(QByteArray aData): mLastLeafTagIndex(0)
{
	setData(aData);
	setCacheSize(cDefaultBuilderCacheSize);
}

//------------------------------------------------------------------------------
void Builder::setData(QByteArray aData)
{
	mData.setData(aData);
	mLastLeafTagIndex = 0;
	mLastPath.clear();
}

//------------------------------------------------------------------------------
void Builder::setCacheSize(uint aSize)
{
	mCache.setMaxCost(aSize);
}

//------------------------------------------------------------------------------
void Builder::clearCache()
{
	mCache.clear();
}

//------------------------------------------------------------------------------
int Builder::build(QString aPath, QDomDocument& aDocument, uint aLeafTagIndex)
{
	QByteArray data;

	if (find(aPath, data, aLeafTagIndex))
	{
		if (aDocument.setContent(data, true, &mLastErrorMessage))
		{
			return data.size();
		}
	}

	return -1;
}

//------------------------------------------------------------------------------
bool Builder::checkCache(QString aPath, qint64& aStartOffset, qint64& aEndOffset) const
{
	TCacheItem *item = mCache[aPath];

	if (item)
	{
		aStartOffset = item->first;
		aEndOffset = item->second;

		return true;
	}

	return false;
}

//------------------------------------------------------------------------------
void Builder::updateCache(QString aPath, qint64 aStartOffset, qint64 aEndOffset) const
{
	mCache.insert(aPath, new TCacheItem(aStartOffset, aEndOffset));
}

//------------------------------------------------------------------------------
bool Builder::find(QString aPath, QByteArray& aData, uint aLeafTagIndex)
{
	CleanPath(aPath);

	if (aPath.isEmpty())
	{
		return false;
	}

	QString tagIndex = QString("[%1]").arg(aLeafTagIndex);

	qint64 startOffset = 0, endOffset = 0, lastOffset = 0;
	bool startFound = false, endFound = false;

	// Check if there's what we need in the cache
	bool inCache = checkCache(aPath+tagIndex, startOffset, endOffset);

	if (inCache)
	{
		startFound = endFound = true;
	}
	else
	{
		int level = 0, globalLevel = 0;
		uint currentLeafTagIndex = aLeafTagIndex;
		QStringList tags;

		// Optimization in case of sequential read
		if (aPath == mLastPath && aLeafTagIndex-1 == mLastLeafTagIndex)
		{
			tags.append(aPath.right(aPath.size()-aPath.lastIndexOf('/')-1));
			currentLeafTagIndex = 0;
		}
		else
		{
			mReader.clear();
			mReader.setDevice(&mData);
			tags = aPath.split('/');
		}

		// Iterate through the data
		while (!mReader.atEnd() && !endFound)
		{
			QXmlStreamReader::TokenType token = mReader.readNext();

			// When a start element is found check if it belongs to the specified path
			if (token == QXmlStreamReader::StartElement)
			{
				if (!startFound && tags.first() == mReader.name() && level == globalLevel)
				{
					// If we've hit, remember its offset
					if (tags.count() == 1)
					{
						// Count the tag number
						if (currentLeafTagIndex)
						{
							--currentLeafTagIndex;
							--level;
						}
						else
						{
							startOffset = lastOffset;
							startFound = true;
						}
					}
					else
					{
						tags.pop_front();
					}

					++level;
				}

				++globalLevel;
			}
			// If a closing element is the alter ego of what we've found before, remember the offset
			else if (token == QXmlStreamReader::EndElement)
			{
				if (startFound && tags.first() == mReader.name() && level == globalLevel)
				{
					endOffset = mReader.characterOffset();
					endOffset = mReader.device()->pos();
					endFound = true;
				}

				--globalLevel;
			}

			lastOffset = mReader.characterOffset()-1;
		}
	}

	if (!startFound)
	{
		mLastErrorMessage = cStartElementNotFound;
	}
	else if (!endFound)
	{
		mLastErrorMessage = cEndElementNotFound;
	}
	else
	{
		// Populate the output buffer if there was no errors
		mLastErrorMessage.clear();
		aData = QByteArray::fromRawData(mData.data().data()+startOffset, endOffset-startOffset);

		if (!inCache)
		{
			updateCache(aPath+tagIndex, startOffset, endOffset);
		}
	}

	mLastPath = aPath;
	mLastLeafTagIndex = aLeafTagIndex;

	return startFound && endFound;
}

//------------------------------------------------------------------------------
QString Builder::lastErrorMessage() const
{
	return mLastErrorMessage;
}

//------------------------------------------------------------------------------
Factory::~Factory()
{
	foreach (TBuilderCachePair pair, mBuilders.values())
	{
		delete pair.first;
		delete pair.second;
	}
}

//------------------------------------------------------------------------------
void Factory::addData(QString aDomain, QByteArray aData)
{
	mBuilders[aDomain] = qMakePair(new Builder(aData), new TDomDocumentCache(cDefaultFactoryCacheSize));
}

//------------------------------------------------------------------------------
bool Factory::deleteData(QString aDomain)
{
	if (hasDomain(aDomain))
	{
		TBuilderMap::iterator item = mBuilders.find(aDomain);
		delete item.value().first;
		delete item.value().second;
		mBuilders.erase(item);

		return true;
	}
	else
	{
		mLastErrorMessage = cNoDomain + aDomain;
	}

	return false;
}

//------------------------------------------------------------------------------
bool Factory::hasDomain(QString aDomain) const
{
	return mBuilders.contains(aDomain);
}

//------------------------------------------------------------------------------
void Factory::setCacheSize(QString aDomain, size_t aSize)
{
	if (hasDomain(aDomain))
	{
		mBuilders[aDomain].second->setMaxCost(aSize);
	}
	else
	{
		mLastErrorMessage = cNoDomain + aDomain;
	}
}

//------------------------------------------------------------------------------
void Factory::clearCache()
{
	foreach (TBuilderCachePair pair, mBuilders.values())
	{
		pair.second->clear();
	}
}

//------------------------------------------------------------------------------
void Factory::clearCache(QString aDomain)
{
	if (hasDomain(aDomain))
	{
		mBuilders[aDomain].second->clear();
	}
	else
	{
		mLastErrorMessage = cNoDomain + aDomain;
	}
}

bool Factory::checkCache(QString aDomain, QString aPath, QDomDocument &aDocument) const
{
	QDomDocument *document = mBuilders[aDomain].second->object(aPath);

	if (document)
	{
		aDocument = *document;
	}

	return document;
}

//------------------------------------------------------------------------------
bool Factory::build(QString aDomain, QString aPath, QDomDocument &aDocument, uint aLeafTagIndex) const
{
	if (hasDomain(aDomain))
	{
		// Check the cache
		CleanPath(aPath);
		QString tagIndex = QString("[%1]").arg(aLeafTagIndex);

		if (!checkCache(aDomain, aPath+tagIndex, aDocument))
		{
			int cost = mBuilders[aDomain].first->build(aPath, aDocument, aLeafTagIndex);

			if (cost >= 0)
			{
				// Update the cache
				mBuilders[aDomain].second->insert(aPath+tagIndex, new QDomDocument(aDocument), cost);

				return true;
			}
		}
		else
		{
			return true;
		}
	}
	else
	{
		mLastErrorMessage = cNoDomain + aDomain;
	}

	return false;
}

//------------------------------------------------------------------------------
bool Factory::find(QString aDomain, QString aPath, QByteArray &aData, uint aLeafTagIndex) const
{
	if (hasDomain(aDomain))
	{
		return mBuilders[aDomain].first->find(aPath, aData, aLeafTagIndex);
	}
	else
	{
		mLastErrorMessage = cNoDomain + aDomain;
	}

	return false;
}

//------------------------------------------------------------------------------
QString Factory::lastErrorMessage(QString aDomain) const
{
	if (hasDomain(aDomain))
	{
		return mBuilders[aDomain].first->lastErrorMessage();
	}
	else
	{
		mLastErrorMessage = cNoDomain + aDomain;
	}

	return mLastErrorMessage;
}

QString Factory::lastErrorMessage() const
{
	return mLastErrorMessage;
}

//------------------------------------------------------------------------------

} // namespade Factory
