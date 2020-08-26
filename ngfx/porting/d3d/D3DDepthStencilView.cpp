/*
 * Copyright 2016 GoPro Inc.
 *
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */
#include "porting/d3d/D3DDepthStencilView.h"
#include "porting/d3d/D3DGraphicsContext.h"
#include "porting/d3d/D3DDebugUtil.h"
using namespace ngfx;

void D3DDepthStencilView::create(D3DGraphicsContext* ctx, uint32_t w, uint32_t h) {
	HRESULT hResult;
	auto device = ctx->d3dDevice.v.Get();
	auto fmt = DXGI_FORMAT_D24_UNORM_S8_UINT;
	
	D3D12_CLEAR_VALUE clearValue = {};
	clearValue.Format = fmt;
	clearValue.DepthStencil.Depth = 1.0f;
	clearValue.DepthStencil.Stencil = 0;

	D3D12_RESOURCE_DESC textureDesc = CD3DX12_RESOURCE_DESC::Tex2D(
		fmt, w, h, 1, 0, 1, 0, D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL
	);
	V(device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&textureDesc,
		D3D12_RESOURCE_STATE_DEPTH_WRITE,
		&clearValue,
		IID_PPV_ARGS(&v)));

	auto& dsvHeap = ctx->d3dDsvDescriptorHeap;
	D3D12_DEPTH_STENCIL_VIEW_DESC desc = {};
	desc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	desc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
	D3D_TRACE(device->CreateDepthStencilView(v.Get(), &desc, dsvHeap.handle.cpuHandle));
	descriptor = dsvHeap.handle.cpuHandle;
	++dsvHeap.handle;
}
