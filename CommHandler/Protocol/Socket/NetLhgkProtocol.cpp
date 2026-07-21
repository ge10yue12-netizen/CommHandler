#include "NetLhgkProtocol.h"
// NetLhgkProtocol.cpp
#include <cstring>
#include <winsock2.h>
#pragma comment(lib, "ws2_32.lib")

NetLhgkProtocol::NetLhgkProtocol(uint8_t version_major, uint8_t version_minor)
	: version_major_(version_major), version_minor_(version_minor) {
}

std::vector<uint8_t> NetLhgkProtocol::buildProtocolNegotiationFrame(bool isRequest) {
	std::vector<uint8_t> payload = {
		static_cast<uint8_t>(isRequest ? 0x01 : 0x02),
		version_major_, version_minor_
	};
	return buildFrame(0x08, payload);
}

std::vector<uint8_t> NetLhgkProtocol::buildControlCommandFrame(uint8_t cmd) {
	return buildFrame(0x02, { cmd });
}

std::vector<uint8_t> NetLhgkProtocol::buildDataChannelControlFrame(uint8_t ctrl) {
	return buildFrame(0x07, { ctrl });
}

std::vector<uint8_t> NetLhgkProtocol::buildModuleDataFrame(const  std::vector < uint8_t>& moduleId, const  std::vector < uint8_t>& dataTypeId, const  std::vector<double>& data)
{
	std::vector<uint8_t> payload;
	appendBigEndian(payload, moduleId.size());
	for (int i = 0; i < moduleId.size(); i++)
	{
		payload.push_back(moduleId.at(i));
		if (i < dataTypeId.size())
		{
			payload.push_back(dataTypeId.at(i));
		}

		appendDoubleBE(payload, data.at(i));
	}

	return buildExternalFrame(0x01, payload);
}

std::vector<uint8_t> NetLhgkProtocol::buildPollingRequestFrame() {
	return buildFrame(0x05, {});
}

std::vector<uint8_t> NetLhgkProtocol::buildExternalDataFrame(double force, double temp) {
	std::vector<uint8_t> payload;

	// 模块段个数
	payload.push_back(0x02); // 两段

	// 力值模块段
	payload.push_back(0x06);      // 模块编号
	payload.push_back(0x0B);      // 数据类型编号（力）
	appendDoubleBE(payload, force);

	// 温度模块段
	payload.push_back(0x06);      // 模块编号
	payload.push_back(0x0C);      // 数据类型编号（温度）
	appendDoubleBE(payload, temp);

	return buildFrame(0x01, payload);
}

std::vector<uint8_t> NetLhgkProtocol::buildStatusResponseFrame(uint8_t statusCode, const std::string& message) {
	std::vector<uint8_t> payload = { statusCode };
	payload.insert(payload.end(), message.begin(), message.end());
	return buildFrame(0x03, payload);
}

std::vector<uint8_t> NetLhgkProtocol::buildErrorFrame(uint8_t errCode, const std::string& errorMsg) {
	std::vector<uint8_t> payload = { errCode };
	payload.insert(payload.end(), errorMsg.begin(), errorMsg.end());
	return buildFrame(0x06, payload);
}

std::vector<uint8_t> NetLhgkProtocol::buildExternalDataRateFrame(uint16_t fps) {
	std::vector<uint8_t> payload = { 0x06 }; // module 0x06
	appendBigEndian(payload, fps);
	return buildFrame(0x09, payload);
}

uint8_t NetLhgkProtocol::getFrameType(const std::vector<uint8_t>& frame) {
	if (frame.size() < 3 || frame[0] != 0xAA || frame[1] != 0x55) return 0xFF;
	return frame[2];
}

void NetLhgkProtocol::getExternalData(const uint8_t& segmentCount, const std::vector<uint8_t>& payload, double& dForce, double& dTemp)
{
	dForce = 0;
	dTemp = 0;
	size_t index = 0;

	for (uint8_t i = 0; i < segmentCount; ++i) {
		if (index + 2 + 8 > payload.size()) return;

		uint8_t moduleId = payload[index++];
		uint8_t typeId = payload[index++];

		uint64_t raw = 0;
		for (int j = 0; j < 8; ++j)
			raw = (raw << 8) | payload[index++];

		double val;
		std::memcpy(&val, &raw, sizeof(val));

		if (moduleId == 0x06) {
			if (typeId == 0x0C) dForce = val;
			else if (typeId == 0x0D) dTemp = val;
		}
	}
}

// 探测完整帧字节数（不含 CRC 校验）；断帧返回 0，非法返回 -1
int NetLhgkProtocol::probeCompleteFrameSize(const std::vector<uint8_t>& buf) const
{
	if (buf.size() < 5)
		return 0;
	if (buf[0] != 0xAA || buf[1] != 0x55)
		return -1;
	const uint8_t type = buf[2];
	const uint16_t length = static_cast<uint16_t>((buf[3] << 8) | buf[4]);
	size_t payloadLen = 0;
	if (type == 0x01)
		payloadLen = static_cast<size_t>(length) * 10;
	else
		payloadLen = static_cast<size_t>(length);
	if (payloadLen > 40960)
		return -1;
	const int total = 9 + static_cast<int>(payloadLen);
	if (static_cast<int>(buf.size()) < total)
		return 0;
	return total;
}

bool NetLhgkProtocol::checkAndParseFrame(const std::vector<uint8_t>& buf, uint8_t& type, std::vector<uint8_t>& payload)
{
	if (buf.size() < 9) return false;

	if (buf[0] != 0xAA || buf[1] != 0x55) return false;
	if (!(buf[buf.size() - 2] == 0x0D && buf[buf.size() - 1] == 0x0A)) return false;

	type = buf[2];
	uint16_t length = (buf[3] << 8) | buf[4];

	size_t payload_start = 5;
	size_t payload_end = buf.size() - 4;  // 去掉 CRC 和帧尾

	size_t expected_payload_len = 0;

	if (type == 0x01) {
		//  模块段个数 × 每段长度（每段固定10字节）
		expected_payload_len = static_cast<size_t>(length) * 10;
	}
	else {
		expected_payload_len = static_cast<size_t>(length);
	}

	if (payload_end - payload_start != expected_payload_len)
		return false;

	//  CRC 校验
	uint16_t expected_crc = (buf[buf.size() - 4] << 8) | buf[buf.size() - 3];
	uint16_t actual_crc = crc16_xmodem(buf.data(), buf.size() - 4);
	if (expected_crc != actual_crc)
		return false;

	// 提取 payload
	payload.assign(buf.begin() + payload_start, buf.begin() + payload_end);
	return true;
}

std::vector<uint8_t> NetLhgkProtocol::buildFrame(uint8_t type, const std::vector<uint8_t>& payload) {
	std::vector<uint8_t> buf = { 0xAA, 0x55, type };
	appendBigEndian(buf, payload.size());
	buf.insert(buf.end(), payload.begin(), payload.end());
	uint16_t crc = crc16_xmodem(buf.data(), buf.size());
	buf.push_back(crc >> 8);
	buf.push_back(crc & 0xFF);
	buf.push_back(0x0D);
	buf.push_back(0x0A);
	return buf;
}

std::vector<uint8_t> NetLhgkProtocol::buildExternalFrame(uint8_t type, const std::vector<uint8_t>& payload)
{
	std::vector<uint8_t> buf = { 0xAA, 0x55, type };
	buf.insert(buf.end(), payload.begin(), payload.end());
	uint16_t crc = crc16_xmodem(buf.data(), buf.size());
	buf.push_back(crc >> 8);
	buf.push_back(crc & 0xFF);
	buf.push_back(0x0D);
	buf.push_back(0x0A);
	return buf;
}

void NetLhgkProtocol::appendBigEndian(std::vector<uint8_t>& buf, uint16_t val) {
	buf.push_back((val >> 8) & 0xFF);
	buf.push_back(val & 0xFF);
}

void NetLhgkProtocol::appendDoubleBE(std::vector<uint8_t>& buf, double d)
{
	uint64_t raw;
	std::memcpy(&raw, &d, sizeof(d));
	// 写入为大端序（高位在前）
	for (int i = 7; i >= 0; --i) {
		buf.push_back(static_cast<uint8_t>((raw >> (i * 8)) & 0xFF));
	}
}

void NetLhgkProtocol::appendFloatBE(std::vector<uint8_t>& buf, float f) {
	uint32_t raw;
	std::memcpy(&raw, &f, sizeof(f));
	raw = htonl(raw);
	buf.insert(buf.end(), {
		static_cast<uint8_t>((raw >> 24) & 0xFF),
		static_cast<uint8_t>((raw >> 16) & 0xFF),
		static_cast<uint8_t>((raw >> 8) & 0xFF),
		static_cast<uint8_t>(raw & 0xFF)
		});
}

uint16_t NetLhgkProtocol::crc16_xmodem(const uint8_t* data, size_t length) {
	uint16_t crc = 0;
	for (size_t i = 0; i < length; ++i) {
		crc ^= ((uint16_t)data[i] << 8);
		for (int j = 0; j < 8; ++j) {
			crc = (crc & 0x8000) ? (crc << 1) ^ 0x1021 : (crc << 1);
		}
	}
	return crc;
}
