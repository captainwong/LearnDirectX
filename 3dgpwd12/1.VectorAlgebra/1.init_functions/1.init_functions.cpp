#include <Windows.h>
#include <DirectXMath.h>
#include <DirectXPackedVector.h>
#include <iostream>

using namespace std;
using namespace DirectX;
using namespace DirectX::PackedVector;

ostream& XM_CALLCONV operator << (ostream& os, FXMVECTOR v)
{
	XMFLOAT3 dest;
	XMStoreFloat3(&dest, v);
	os << "(" << dest.x << ", " << dest.y << ", " << dest.z << ")";
	return os;
}

int main()
{
	if (!XMVerifyCPUSupport()) {
		cout << "directx math not supported" << endl;
		return 0;
	}

	auto p = XMVectorZero();
	auto q = XMVectorSplatOne();
	auto u = XMVectorSet(1.0f, 2.0f, 3.0f, 0.0f);
	auto v = XMVectorReplicate(-2.0f);
	auto w = XMVectorSplatZ(u);

	cout << "p=" << p << endl;
	cout << "q=" << q << endl;
	cout << "u=" << u << endl;
	cout << "v=" << v << endl;
	cout << "w=" << w << endl;
}
