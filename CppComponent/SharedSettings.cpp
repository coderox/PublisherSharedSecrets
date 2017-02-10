#pragma once

#include <ppltasks.h>
#include "pch.h"
#include "SharedSettings.h"

using namespace CppComponent;

using namespace Platform;
using namespace Windows::Storage;
using namespace Windows::Storage::Streams;
using namespace Windows::Foundation;
using namespace Windows::Foundation::Collections;
using namespace Windows::Security::Cryptography;
using namespace Windows::Security::Cryptography::Core;
using namespace Windows::System::UserProfile;
using namespace concurrency;

SharedSettings::SharedSettings() 
	: mFolderName("SharedSettings")
{

}

SharedSettings::SharedSettings(String^ folderName) 
	: mFolderName(folderName)
{
	
}

IAsyncAction^ SharedSettings::ClearAsync() {
	return create_async([=]() {
		return create_task(ApplicationData::Current->ClearPublisherCacheFolderAsync(mFolderName))
			.then([=](task<void> t) {
			try
			{
				t.get();
				ApplicationData::Current->LocalSettings->DeleteContainer(mFolderName);
				OutputDebugStringW(L"Cleared\n");
			}
			catch (Exception^ ex)
			{

			}
		});
	});
}

IAsyncOperation<String^>^ SharedSettings::GetAsync(String^ key)
{
	auto storageFolder = ApplicationData::Current->GetPublisherCacheFolder(mFolderName);
	
	return create_async([=]() -> task<String^> {
		return create_task(ReadAsync(storageFolder, key))
			.then([=](String^ value) {
			if (value != nullptr && value->Length() > 0) {
				return create_task(WriteAsync(storageFolder, key, value))
					.then([value]() -> String^ {
					return value;
				});
			}
			else {
				return create_task([]() -> String^ { return nullptr; });
			}
		});
	});
}

String^ SharedSettings::Get(String^ key) {
	return create_task(GetAsync(key)).get();
}

IAsyncAction^ SharedSettings::SetAsync(String^ key, String^ value) {
	auto storageFolder = ApplicationData::Current->GetPublisherCacheFolder(mFolderName);
	return WriteAsync(storageFolder, key, value);
}

void SharedSettings::Set(String^ key, String^ value) {
	create_task(SetAsync(key, value)).get();
}

IAsyncOperation<String^>^ SharedSettings::GetDeviceIdAsync()
{
	auto storageFolder = ApplicationData::Current->GetPublisherCacheFolder(mFolderName);
	String^ key = "DeviceId";
	return create_async([=]() -> task<String^> {		
		return create_task(ReadAsync(storageFolder, key))
			.then([=](String^ value) {
			if (value == nullptr || value->Length() == 0) {
				value = Generate();
			}
			return create_task(WriteAsync(storageFolder, key, value))
				.then([value]() -> String^ {
				return value;
			});
		});
	});
}

IAsyncOperation<String^>^ SharedSettings::ReadAsync(StorageFolder^ storageFolder, String^ key) {
	return create_async([=]() -> task<String^> {
		String^ valueInSettings = ReadFromSettings(key);
		if (valueInSettings->Length() == 0) {
			return create_task(ReadFromCacheAsync(storageFolder, key));
		}
		else {
			return create_task([valueInSettings] {
				return valueInSettings;
			});
		}
	});
}

IAsyncAction^ SharedSettings::WriteAsync(StorageFolder^ storageFolder, String^ key, String^ value)
{
	WriteToSettings(key, value);
	return WriteToCacheAsync(storageFolder, key, value);
}


IAsyncOperation<String^>^ SharedSettings::ReadFromCacheAsync(StorageFolder^ storageFolder, String^ fileName) {
	return create_async([=]()-> task<String^> {
		return create_task(storageFolder->TryGetItemAsync(fileName))
			.then([=](IStorageItem^ storageItem) {
			auto storageFile = dynamic_cast<StorageFile^>(storageItem);
			if (storageFile != nullptr) {
				OutputDebugStringW(L"Cache hit!\n");
				return create_task(FileIO::ReadTextAsync(storageFile));
			}
			else {
				OutputDebugStringW(L"Cache miss!\n");
				return create_task([]() { return ref new String(); });
			}
		});
	});
}

IAsyncAction^ SharedSettings::WriteToCacheAsync(StorageFolder^ storageFolder, String^ key, String^ value) {
	return create_async([=]()
	{
		return create_task(storageFolder->TryGetItemAsync(key))
			.then([=](IStorageItem^ storageItem) {
			auto storageFile = dynamic_cast<StorageFile^>(storageItem);
			if (storageFile != nullptr) {
				return create_task([storageFile]() {
					return storageFile;
				});
			}
			else {
				return create_task(storageFolder->CreateFileAsync(key))
					.then([](StorageFile^ storageFile) {
					return storageFile;
				});
			}
		})
			.then([=](StorageFile^ storageFile) {
			return create_task(FileIO::WriteTextAsync(storageFile, value))
				.then([] {
				OutputDebugStringW(L"Wrote to cache!\n");
			});
		});
	});
}

String^ SharedSettings::ReadFromSettings(String^ key) {
	auto localSettings = ApplicationData::Current->LocalSettings;

	ApplicationDataContainer^ container = localSettings->CreateContainer(mFolderName, ApplicationDataCreateDisposition::Always);

	if (localSettings->Containers->HasKey(mFolderName))
	{
		auto values = localSettings->Containers->Lookup(mFolderName)->Values;
		if (values->HasKey(key)) {
			OutputDebugStringW(L"Settings hit!\n");
			return safe_cast<Platform::String^>(values->Lookup(key));
		}
	}
	OutputDebugStringW(L"Settings miss!\n");
	return nullptr;
}

void SharedSettings::WriteToSettings(String^ key, String^ value) {
	auto localSettings = ApplicationData::Current->LocalSettings;

	ApplicationDataContainer^ container = localSettings->CreateContainer(mFolderName, ApplicationDataCreateDisposition::Always);

	if (localSettings->Containers->HasKey(mFolderName))
	{
		auto values = localSettings->Containers->Lookup(mFolderName)->Values;
		values->Insert(key, value);
		OutputDebugStringW(L"Wrote to settings!\n");
	}
}

static String^ GenerateGuid() {
	GUID nativeGuid;
	CoCreateGuid(&nativeGuid);
	Guid guid(nativeGuid);
	return guid.ToString();
}

String^ SharedSettings::Generate()
{
	// First try with advertising id as base for id
	auto id = AdvertisingManager::AdvertisingId;
	if (id == nullptr || id->Length() == 0) {
		// Secondly generate a new guid as base for id
		id = GenerateGuid();
	}

	auto algorithm = HashAlgorithmProvider::OpenAlgorithm("MD5");
	auto vector = CryptographicBuffer::ConvertStringToBinary(id, BinaryStringEncoding::Utf8);
	auto buffer = algorithm->HashData(vector);

	if (buffer->Length != algorithm->HashLength) {
		throw ref new Platform::Exception(0, "Failed to generate hash!");
	}

	return CryptographicBuffer::EncodeToHexString(buffer);
}
