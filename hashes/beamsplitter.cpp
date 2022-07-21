/*
 * Beamsplitter hash
 * Copyright (C) 2021-2022  Frank J. T. Wojcik
 * Copyright (c) 2020-2021 Reini Urban
 * Copyright (c) 2020 Cris Stringfellow
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use, copy,
 * modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */
#include "Platform.h"
#include "Hashlib.h"

// gotten from random.org
// as hex bytes that I formatted into 64-bit values
static const uint64_t T[1024] = {
    UINT64_C(0x6fa74b1b15047628), UINT64_C(0xa2b5ee64e9e8f629), UINT64_C(0xd0937853bdd0edca), UINT64_C(0x4e9fb2b2b0a637a6),
    UINT64_C(0x26ac5a8fac69497e), UINT64_C(0x51e127f0db14aa48), UINT64_C(0xea5b9f512d8d6a09), UINT64_C(0xf3af1406a87de6a9),
    UINT64_C(0x3b36e2ed14818955), UINT64_C(0xb0ac19ef2dde986c), UINT64_C(0xd34ed04929f8f66d), UINT64_C(0xe99978cff2b324ea),
    UINT64_C(0x4032cb3ecff8cb38), UINT64_C(0xfa52274072d86042), UINT64_C(0x27437346dec26105), UINT64_C(0xec1cbf04b76aec71),
    UINT64_C(0x6dd57b3dac56cd39), UINT64_C(0x34e9021797e95aad), UINT64_C(0xdc8d3363540c5999), UINT64_C(0x773d283eeeabf4ab),
    UINT64_C(0x373c522657461aaf), UINT64_C(0x154cfe0f497d7f78), UINT64_C(0x6d377183b5ca6550), UINT64_C(0x614da5f6055e904b),
    UINT64_C(0xd77b66b34896f00e), UINT64_C(0x122538125d6adaef), UINT64_C(0x1021e161206d9091), UINT64_C(0x38407c4313aefdfa),
    UINT64_C(0xd941cc5dafc66162), UINT64_C(0xfc2432a6ea885315), UINT64_C(0x5576dc02b68b10ed), UINT64_C(0xd8449f9d4ab139a2),
    UINT64_C(0xd333cbcd49cbacba), UINT64_C(0x700d20430e06eeb8), UINT64_C(0xdeb34810d6d0320a), UINT64_C(0x6743363d6cc8ba68),
    UINT64_C(0xbd183cb526e6e936), UINT64_C(0xee62bf5ee97de5ea), UINT64_C(0xf6b855e743e76853), UINT64_C(0x83ac16a35d132df9),
    UINT64_C(0x2046f2c70c2130b1), UINT64_C(0xaadd5007102b5ee4), UINT64_C(0x8eedac842e63cdac), UINT64_C(0xba02956e43c18608),
    UINT64_C(0xd2688af010adbeaf), UINT64_C(0x4aaa5295377c17be), UINT64_C(0x83792382ba198f10), UINT64_C(0x6fc42849961a25b6),
    UINT64_C(0x3501677f06fb1311), UINT64_C(0x1e18b89705c224dd), UINT64_C(0xa0a0b8684aa2e12d), UINT64_C(0x30d19aac3d40898e),
    UINT64_C(0x41dd335a29272e9b), UINT64_C(0x5c5d445a07426e3f), UINT64_C(0x6f13080e67946fdc), UINT64_C(0x3ddabae21609bf08),
    UINT64_C(0x8e6146d3cde11ca5), UINT64_C(0x9eff76a4c39eacf4), UINT64_C(0x71c66d0a423a21b7), UINT64_C(0x68515c0b712bbc4f),
    UINT64_C(0x5edd17cec412a735), UINT64_C(0xa444f487c96f896c), UINT64_C(0xc161d16d4e54041a), UINT64_C(0x3a2d84d3e09bafb9),
    UINT64_C(0x63a406b157a5f2f1), UINT64_C(0x18292d6007f839ba), UINT64_C(0xcaac5789618f2aac), UINT64_C(0x6f516d95f749dd97),
    UINT64_C(0xb5784409560e219f), UINT64_C(0x12f0f0d6fbdcb81c), UINT64_C(0x993d6c2a47089679), UINT64_C(0xcc9247b35870aebf),
    UINT64_C(0xa1ca8eff8b1bca70), UINT64_C(0x7a1d015397e558cc), UINT64_C(0xc504a4d4815f8722), UINT64_C(0x3e44258e93472b26),
    UINT64_C(0x11bd0578a36c8044), UINT64_C(0x84c7087603a0a6ea), UINT64_C(0x457d0c59e84c9ac8), UINT64_C(0x32129275ee63dd95),
    UINT64_C(0x66269220e943024d), UINT64_C(0x197de12f9d6e5c72), UINT64_C(0x06fdd09a4d6157dd), UINT64_C(0xf8c1a8b51fe95716),
    UINT64_C(0x41eeb6129149f6cf), UINT64_C(0x42f510887a61de1b), UINT64_C(0xf3d2aa6e4fe5949d), UINT64_C(0xc0799007b85373aa),
    UINT64_C(0x81577b167de515c3), UINT64_C(0x01f424fc6b856270), UINT64_C(0xff6247ed0658caa8), UINT64_C(0x63ad005e620fe4bb),
    UINT64_C(0xdb919b9f63c93174), UINT64_C(0x5693dbd6c76c7683), UINT64_C(0xdaa9b82e85e0355a), UINT64_C(0x424c5c4e5672fc73),
    UINT64_C(0x9de3ca332ba818f1), UINT64_C(0xb28f375a58bc6c1e), UINT64_C(0xef0af1e6041b9cd4), UINT64_C(0x0418afb53ef5408f),
    UINT64_C(0x9a37634585d3330a), UINT64_C(0x3ab5aec014b097cd), UINT64_C(0x384a0739a3ff7dc8), UINT64_C(0x0ff31c11226e5d5a),
    UINT64_C(0x71070735f1c16bb4), UINT64_C(0xc4f78905f49a3840), UINT64_C(0x561f68d6a5f44a81), UINT64_C(0xb09bd8cd8d932357),
    UINT64_C(0xf270b47652354fdb), UINT64_C(0x47d6ca7bba50c2c7), UINT64_C(0x2720590d7b2b7b54), UINT64_C(0xcaac35df08cab300),
    UINT64_C(0xd05759dee169d9fd), UINT64_C(0xdb8d0d0403a6aafb), UINT64_C(0xcd3ab85684ba537c), UINT64_C(0xad69c4e5240c158f),
    UINT64_C(0x65427c4ff3637db2), UINT64_C(0x085ecbbf903a45ae), UINT64_C(0xeafed57a94384c62), UINT64_C(0xc99972367cd21eba),
    UINT64_C(0xc1e2cf52270b20eb), UINT64_C(0x825dad5142681653), UINT64_C(0x47e99edc5e141d94), UINT64_C(0x125813bc26e42e07),
    UINT64_C(0x06f41d2441b172ca), UINT64_C(0x5e9e640ed911730e), UINT64_C(0x5900403342f0f362), UINT64_C(0x57a600d157ee9945),
    UINT64_C(0xbcc5d702f02dc7e0), UINT64_C(0x8258cf5a1a6435ab), UINT64_C(0xdf885b6a0343a3e0), UINT64_C(0xadd74c04a503b09a),
    UINT64_C(0x0ea210122eeef589), UINT64_C(0x5217fd50f3ecaf85), UINT64_C(0xd0c39849df6b4756), UINT64_C(0xf66d9e1c91bd0981),
    UINT64_C(0x0f355b00f40e3e6b), UINT64_C(0xc01dabcd14518520), UINT64_C(0x58691b4fa9e7d327), UINT64_C(0x357616c77c22fffe),
    UINT64_C(0xb9fbf8de2ed23303), UINT64_C(0x0195932bc205c466), UINT64_C(0xef0763590a08a50d), UINT64_C(0xf546866c0028a938),
    UINT64_C(0x41cc8732eaad496a), UINT64_C(0xadc61f16374896c6), UINT64_C(0x5eb8f93f25ad0457), UINT64_C(0x240f00f5db3fae25),
    UINT64_C(0xcc48503596dc01ef), UINT64_C(0x351baaa904a306d5), UINT64_C(0x7111179ae328bb19), UINT64_C(0x6789a31719d5d453),
    UINT64_C(0xf5318492c9613de6), UINT64_C(0xa0e8c24f3f0da716), UINT64_C(0xac15d68d54401b9d), UINT64_C(0xadafb35cf63092ee),
    UINT64_C(0xceb5f8d63c7fec4c), UINT64_C(0x1ae71929b980fc9d), UINT64_C(0x6efdc5693ef4ee2a), UINT64_C(0xbedd8334cade7855),
    UINT64_C(0x06f1b768b476a249), UINT64_C(0x9e614bedf41dd639), UINT64_C(0x9eca9c6c9e389a5d), UINT64_C(0x76999bf01b912df2),
    UINT64_C(0x04d52fb2ac70ab31), UINT64_C(0xe467ea8172f5d066), UINT64_C(0x356ed51bb0e094ae), UINT64_C(0xab2047c21b54d8ba),
    UINT64_C(0x21dbbfa0a6157474), UINT64_C(0x7de36edec62f1997), UINT64_C(0x306ef59f5204a58c), UINT64_C(0x954135a769d5b72e),
    UINT64_C(0x9d7774a0c2d29380), UINT64_C(0xc03acfd63ac6b88c), UINT64_C(0x9989d5ee565322e6), UINT64_C(0x19d1a58324bdd145),
    UINT64_C(0xe74685383cc6b27c), UINT64_C(0xf9edffe1c4d81108), UINT64_C(0x94950b5b6247cb43), UINT64_C(0xe3fa8c6468d419eb),
    UINT64_C(0x29981bd802f77ac5), UINT64_C(0x6cf1a6cab28c1c36), UINT64_C(0x1d34a382a5d48973), UINT64_C(0xcd1d5d546e5e4d3d),
    UINT64_C(0x4ad78b4a37e52322), UINT64_C(0x24da17671ab463f2), UINT64_C(0x527504b7c7bc5537), UINT64_C(0x7ba1d92e1969b2b5),
    UINT64_C(0x53a130812c49d64a), UINT64_C(0x503af48d9510f1d7), UINT64_C(0x719db8a348dee165), UINT64_C(0xa85e4fad1f343e67),
    UINT64_C(0xdafc1fa9203d2d45), UINT64_C(0x7730f245c903a407), UINT64_C(0xb7c04e53f913aeae), UINT64_C(0x39ed817e1e039153),
    UINT64_C(0xf415ea2b3efc7606), UINT64_C(0x15e3c53fe43f104d), UINT64_C(0x1b71e4d83ccba83c), UINT64_C(0xfe088f4c90812841),
    UINT64_C(0x1ff8e2ee0a04b6ae), UINT64_C(0xf4f4a23612b9eed2), UINT64_C(0xc596a66051b8aca1), UINT64_C(0xbc898edd3370a8dd),
    UINT64_C(0xce7638a7a2f9152e), UINT64_C(0xd99192635c0d5c92), UINT64_C(0x62038c87c094a1ff), UINT64_C(0xa73f1bcaac7343af),
    UINT64_C(0x93c797804faa5ff3), UINT64_C(0x9da7407c705da1f0), UINT64_C(0xa52cde7d37fef9f0), UINT64_C(0xb93a7db97e3fa7ff),
    UINT64_C(0x75ee91392c60fb6b), UINT64_C(0x4d7f8e3db9383ae0), UINT64_C(0xe0aec397d5290d06), UINT64_C(0x159a20f22d740d81),
    UINT64_C(0x231416cff9a9b014), UINT64_C(0x71ed3a6e513b4795), UINT64_C(0x190b08ebcb87f3bc), UINT64_C(0x36bb0bcb0e8df593),
    UINT64_C(0xc1e63cdc4d78dfb3), UINT64_C(0x36e2c57ba6799460), UINT64_C(0x280c0618b19f63dc), UINT64_C(0xca2b8e49d6c71d2d),
    UINT64_C(0xc881e59705270f09), UINT64_C(0x26fdf0dbb5f2f451), UINT64_C(0xc6d1a3697ca86855), UINT64_C(0xd00755a203980eb5),
    UINT64_C(0xa85962163dd7de95), UINT64_C(0x622b7a1d2531d00e), UINT64_C(0xb6c1cfba74436ef7), UINT64_C(0x9578891a720bf317),
    UINT64_C(0x5e325058bd3a343a), UINT64_C(0x9a468a5a888a475f), UINT64_C(0xa57f0edb414a0589), UINT64_C(0xa044aef7ea680f8c),
    UINT64_C(0x2036717cee9b991a), UINT64_C(0x3925631ec66cb8aa), UINT64_C(0xdcb6a5da6b2fc78f), UINT64_C(0x17a8cd724b7b5e26),
    UINT64_C(0x1c704c6a48a2dae0), UINT64_C(0x87d8f6738a0c30bc), UINT64_C(0xd8580262a4801240), UINT64_C(0x5812cea521ffaeaf),
    UINT64_C(0x21b6ff923871f14c), UINT64_C(0x922dbd45c2b307d1), UINT64_C(0x5c67ecbaace24d31), UINT64_C(0xb90f5e3acfaeff9b),
    UINT64_C(0xea5aa9f2f14efeb1), UINT64_C(0x08003af95ab5ce92), UINT64_C(0x5a39361e05692622), UINT64_C(0xd4b8cddc309e44da),
    UINT64_C(0xe20bfe5f0a1343d9), UINT64_C(0x13848357d100b2b3), UINT64_C(0x912a1b220fa678f5), UINT64_C(0x7631242b7f6d6365),
    UINT64_C(0x5a9f9a3284d95674), UINT64_C(0x0d5b02c98afd4279), UINT64_C(0xede70dbc04a7a3d9), UINT64_C(0xadb3f72865ba580e),
    UINT64_C(0xc4a3c11163562e90), UINT64_C(0x482e567c69b6b128), UINT64_C(0x38ec96bfcb4d965d), UINT64_C(0x923fe02a6b4bdabe),
    UINT64_C(0x0ae0ca91a2be0579), UINT64_C(0x137401e7f2acf3e8), UINT64_C(0xfdad100e85bc5622), UINT64_C(0x9c07483343c8030f),
    UINT64_C(0x71872f8555dbd0a8), UINT64_C(0x8de5873dbfa538e0), UINT64_C(0x2922d0d9a2d9eb02), UINT64_C(0x2744006cfc375d0c),
    UINT64_C(0xa82c09537574f583), UINT64_C(0x2ab2d255e73f6f83), UINT64_C(0x6cc5f73b682b3701), UINT64_C(0x6e59fc51ee28845d),
    UINT64_C(0xe536b381533cc4cf), UINT64_C(0xfd2ac9f30025e109), UINT64_C(0xc26cdfa60b8be153), UINT64_C(0x62da136e08f0f885),
    UINT64_C(0xeb6a7a065b640357), UINT64_C(0x7462b101e2adb3ff), UINT64_C(0x996ec340bf52ea07), UINT64_C(0xf0aa2a872333e60c),
    UINT64_C(0x222884f9c4632341), UINT64_C(0x32b5289d94dac82e), UINT64_C(0x7cdd99055bd35f17), UINT64_C(0x92d3d262aefe21bc),
    UINT64_C(0xc6c1b1029eb0dd4c), UINT64_C(0x28f046ec80f3c975), UINT64_C(0xc1f0c2d9745c5cb7), UINT64_C(0x92ada28cf6f7fe0b),
    UINT64_C(0xdfb215a8df753a03), UINT64_C(0x942ecdad535f962d), UINT64_C(0x7d739b8c0b7a1669), UINT64_C(0xee95286e88be8510),
    UINT64_C(0x4ae71aa9d3c3d36f), UINT64_C(0x2bd6d5d12452cc38), UINT64_C(0x16fa1504fbedf267), UINT64_C(0x4b835f8377f3937d),
    UINT64_C(0x0004374053160cb7), UINT64_C(0xe44a676c90906fe8), UINT64_C(0x2389c459f53fbdcd), UINT64_C(0x4a7031455481da9e),
    UINT64_C(0xb72c293d969a40cc), UINT64_C(0xd9b72ee09dde404d), UINT64_C(0xa31f4f98c5aabc97), UINT64_C(0x56f240ad0aea491c),
    UINT64_C(0x86264ebf858d67bf), UINT64_C(0x93fd3b332948fd87), UINT64_C(0x79899120e2d72215), UINT64_C(0x36dedea1a614643e),
    UINT64_C(0x1c5e947b88cba0f6), UINT64_C(0x20ec77907c771a4f), UINT64_C(0x587a65fe2c8f5487), UINT64_C(0x9b5431d881ff3b4a),
    UINT64_C(0x8f55b2fd967902d7), UINT64_C(0xebd59a640fee9b7e), UINT64_C(0xd5a77b39543d5bef), UINT64_C(0x5dbf440d204f5d0f),
    UINT64_C(0x4e22065f53ba213e), UINT64_C(0x4611a2d169ad5a0b), UINT64_C(0x41ea9888cb5be7d1), UINT64_C(0xf8a661f2359be997),
    UINT64_C(0xde83a9e3a6562631), UINT64_C(0xd66dedc223dad775), UINT64_C(0x162e54732874a52a), UINT64_C(0xf6d91b1963c23d56),
    UINT64_C(0x56d3c9a025a95772), UINT64_C(0x92ddff0a1caeb05c), UINT64_C(0x6cbeb9f263443bd7), UINT64_C(0xb4ad540e1b11894b),
    UINT64_C(0xcfa573f2f78d8b29), UINT64_C(0xad477ed16d45543f), UINT64_C(0x0d0283973ed3423a), UINT64_C(0x5307f93f3654f284),
    UINT64_C(0xbc9b362f504b145b), UINT64_C(0x5661193dc5bcb5ff), UINT64_C(0x151c9b1c7c0f246a), UINT64_C(0xad25cfcfd5e399d2),
    UINT64_C(0xc5855adf08226db2), UINT64_C(0x5a027c03c078be13), UINT64_C(0xc2465bfb0dc5b99c), UINT64_C(0x8aaa55a9eca79b60),
    UINT64_C(0x797a7c2608c23d9e), UINT64_C(0x692b8d7da8c7f748), UINT64_C(0xc23c7b1ab3e883e1), UINT64_C(0xe1ebb866f32ac6cf),
    UINT64_C(0xca6be5075b5046f9), UINT64_C(0x3105a0555f6a3bac), UINT64_C(0x525b7cc4839ea6c5), UINT64_C(0xce1dd2aad7e83cf1),
    UINT64_C(0xb4a9105674d79be6), UINT64_C(0x667eb8384834f7db), UINT64_C(0xb200a7a30f789150), UINT64_C(0x4ba4d2c780055821),
    UINT64_C(0xb48a01ad5f7474c6), UINT64_C(0x3310ba4a1e25aab8), UINT64_C(0x64379d2408fd5735), UINT64_C(0xf11e9788704e5e0d),
    UINT64_C(0xe9866ab0a8e90f4e), UINT64_C(0xaa344ffe50f7a934), UINT64_C(0xcce37a15b3870924), UINT64_C(0xe22135597a867f1c),
    UINT64_C(0x8770a58d7fe57f99), UINT64_C(0xcafbbc8d2024bcbc), UINT64_C(0x2307e7f0fcdb1909), UINT64_C(0xdd016550b9ed2b2a),
    UINT64_C(0xd0bcf0e9dee7df90), UINT64_C(0xe82d2e7daeab325c), UINT64_C(0x721a2aba71709aa7), UINT64_C(0x38cfabc260602614),
    UINT64_C(0x3099ccb02b73b4c8), UINT64_C(0x00250ce48fd67df0), UINT64_C(0xcace64d8984b19cf), UINT64_C(0xee305dcbae8615ca),
    UINT64_C(0xd187da55485b86ef), UINT64_C(0xebea32b2455e6486), UINT64_C(0x77cb912fa927d5c5), UINT64_C(0x911002ac8b62cbd8),
    UINT64_C(0x70730c24c32c5870), UINT64_C(0x0a7cb6f89e988a83), UINT64_C(0x6b5e00839b7db787), UINT64_C(0xecae9f4cfd9ce924),
    UINT64_C(0xae09926b714019a5), UINT64_C(0xbc1b2c59bc5ce769), UINT64_C(0x592756761e90349f), UINT64_C(0x95c9a69a21936de3),
    UINT64_C(0x192b2119ee48eb9a), UINT64_C(0xcd8d11ebcd8a71c2), UINT64_C(0x34de8d4cad3151d6), UINT64_C(0x0fc4f3baf540eb1c),
    UINT64_C(0x88bd85e02b2ec0e2), UINT64_C(0x5b65423e815dafb6), UINT64_C(0x66ec6fadd29f273e), UINT64_C(0xc3622fbc1f1c7bd0),
    UINT64_C(0x50cc102827ff1acf), UINT64_C(0xe73cab705018a55f), UINT64_C(0xcd552b588a227f38), UINT64_C(0xc462735f28a9c597),
    UINT64_C(0x3e3ccb00a16906e1), UINT64_C(0x79bdf5d7e7dfa593), UINT64_C(0xb333b6942d5db3a9), UINT64_C(0x3566edd901f25f20),
    UINT64_C(0x8c5fe3e063253c7b), UINT64_C(0x9f0aa4160fb652ee), UINT64_C(0x2361d9bca2c92f43), UINT64_C(0x2d6a0339fe1de8ee),
    UINT64_C(0x389b1bd9476b0470), UINT64_C(0xd7fa2522f0da451e), UINT64_C(0x43e6a01d67c62b2d), UINT64_C(0x5bdc15971dc0d5b3),
    UINT64_C(0x38a0a80acbadf021), UINT64_C(0x2c66125ec66e1fad), UINT64_C(0xb58f61bb53b6a9ff), UINT64_C(0x492142919b2d61d6),
    UINT64_C(0xd905263cc927ebd9), UINT64_C(0xca15f966e2279122), UINT64_C(0xf9dc67f8101119c9), UINT64_C(0x7f6755699c23d8c9),
    UINT64_C(0x26146d38a23b0bdf), UINT64_C(0x0166c70bc773d9aa), UINT64_C(0x5b3317113904ec75), UINT64_C(0x5d3c4311b21e44d1),
    UINT64_C(0x479c13c75df8cf18), UINT64_C(0x75a880dd38a8a4ff), UINT64_C(0xdf378e2eb432708d), UINT64_C(0xca1cb0f76b1c5f04),
    UINT64_C(0x06c76e876516eb46), UINT64_C(0x965c10e60ec202ad), UINT64_C(0x67b18e2140e0aad3), UINT64_C(0x203ca38572b212b8),
    UINT64_C(0x72adad835dd333c6), UINT64_C(0xdd02aa349680a96a), UINT64_C(0x69ab0df01d4b3eab), UINT64_C(0xfebfd83a2c43afd1),
    UINT64_C(0x0dcd90c392b9fae4), UINT64_C(0x8a87b8033e4cd8cc), UINT64_C(0x3902150c36e99880), UINT64_C(0xb5b655e071474ebc),
    UINT64_C(0x6c2dc9eeaffbd8d8), UINT64_C(0x3cf62bfa4986f0fe), UINT64_C(0xa68eaf0719a9afbc), UINT64_C(0xde1f4e9a4b190aef),
    UINT64_C(0x7fbc9e8538999e56), UINT64_C(0xf6d5e9db2208a40c), UINT64_C(0x93b13abaddf4554c), UINT64_C(0xd8b5e4ad9911629f),
    UINT64_C(0x6fdb9d7376488e52), UINT64_C(0xee604a7ce20d75ad), UINT64_C(0x94ec4abbaa9c2c1d), UINT64_C(0xdbd148c4fcd05ec1),
    UINT64_C(0x0865c7c3b380a005), UINT64_C(0xa6da59a56992f211), UINT64_C(0x2eb1dc9f941c83ef), UINT64_C(0x3bf5ccf06910fae7),
    UINT64_C(0x23a70e117e1f29f0), UINT64_C(0x4273791acbf6c4e5), UINT64_C(0x338414ec6b5e5d60), UINT64_C(0xa5873517e3d057d9),
    UINT64_C(0xea88400a890764f6), UINT64_C(0xc0569d573ca5364f), UINT64_C(0x4c3fc02fc93316e0), UINT64_C(0x76597f718657e577),
    UINT64_C(0x17052b8440c7d824), UINT64_C(0x9a7ec0a30be21a00), UINT64_C(0xab0453ac2173dac9), UINT64_C(0xb6f3706820512809),
    UINT64_C(0xef44f0b07d46180a), UINT64_C(0x5e9aa12e99509a72), UINT64_C(0x6231337efc0182ca), UINT64_C(0x0963321a419da89b),
    UINT64_C(0xfda3e7ad51f82b5e), UINT64_C(0x1ab8790c2f5bf1a3), UINT64_C(0x9ef177b8a59f28c0), UINT64_C(0x27d1c87da66c1652),
    UINT64_C(0x1bd6bdf27c49d109), UINT64_C(0xc151e2a66994d599), UINT64_C(0x5e1b8d826b8c12a9), UINT64_C(0x39f41d57213261b5),
    UINT64_C(0x16a57bd0bc78aada), UINT64_C(0x0127e7f9699b55c7), UINT64_C(0xd79eccc9f9d703be), UINT64_C(0xb41b81c61ba66d7d),
    UINT64_C(0xcf8b79dcb95dce93), UINT64_C(0x5ca102a7743a6e0d), UINT64_C(0xf422a0c3a2ad7b28), UINT64_C(0x4a9137b4a0f03724),
    UINT64_C(0x907dcf6425c829c2), UINT64_C(0x15551fd4432261fb), UINT64_C(0xa057dfbd55ef436c), UINT64_C(0x8b2541b9e0e0fa7e),
    UINT64_C(0x7262166dcdf4b67e), UINT64_C(0xcf6533e5c608aaeb), UINT64_C(0xd6763d3967359786), UINT64_C(0x1f6b0228d257c676),
    UINT64_C(0xc268c1064d2b458a), UINT64_C(0x6d8b2f6e75d2b613), UINT64_C(0xfaaf5adc43d72807), UINT64_C(0xb6376765e344f9f8),
    UINT64_C(0xa8e18dd16a4bd501), UINT64_C(0xa71aa12a8ec11351), UINT64_C(0x1daaf130b537ebe0), UINT64_C(0x2e8aa415959d5d8f),
    UINT64_C(0x2813ff3a3e5cbcfb), UINT64_C(0xf0fdd1d6d16a7c23), UINT64_C(0xbf2b55d2ecf0ee55), UINT64_C(0xbd4e9bec299381d0),
    UINT64_C(0xac8827ab807eb180), UINT64_C(0x8514d75ac3b43b0b), UINT64_C(0xc9b5c78e45fb38a8), UINT64_C(0x4b66e6e7b797cd8f),
    UINT64_C(0x1a482ffa6870a2d3), UINT64_C(0x98f55f701d4bf919), UINT64_C(0x7c0fda20e7e26ef8), UINT64_C(0x6ef795976fca3b54),
    UINT64_C(0x79801cd422fa95cd), UINT64_C(0xce8a72301dbbe230), UINT64_C(0x5e79f4c925bdd0e0), UINT64_C(0x5729e93c99cc12b3),
    UINT64_C(0x76d022747522392a), UINT64_C(0xb9d7652e917a6bc4), UINT64_C(0xc2978462dfa9551b), UINT64_C(0xac081b4a7528b0ce),
    UINT64_C(0x5b7799fe02443b33), UINT64_C(0x6676e5687742e76a), UINT64_C(0x3e9836e33caf452b), UINT64_C(0x96ff93e427173943),
    UINT64_C(0x30fa2f987359e0f6), UINT64_C(0xfaa730326c478363), UINT64_C(0x2bb0560d8986947e), UINT64_C(0x9f7c01d35aefc68f),
    UINT64_C(0x6b81189bd90a0e45), UINT64_C(0xd592d2ad2df04128), UINT64_C(0xbcd0e0fe02816ec6), UINT64_C(0x1d6d84e5c1f8df0f),
    UINT64_C(0xc4b55a73da2f8713), UINT64_C(0xdbd6510e7ad24d26), UINT64_C(0x7e3452b770e259bd), UINT64_C(0xd5fe716f2c3ee835),
    UINT64_C(0x63a6d74ef78acd1d), UINT64_C(0x3bd673b27d5aa140), UINT64_C(0xe394f3a2a4f6d465), UINT64_C(0xf02f642cda7fee7e),
    UINT64_C(0xe17ee2617b3d366a), UINT64_C(0x41cdb92402dce780), UINT64_C(0x4e5c54024fd18f6b), UINT64_C(0x6f45dd1c7c5a3f12),
    UINT64_C(0xf6fd2b3f9ccda563), UINT64_C(0xe7628d358d971e26), UINT64_C(0x4dabc984370ed105), UINT64_C(0xec05f7d5c53cb70b),
    UINT64_C(0xf48eccbc216dcf71), UINT64_C(0x8a571d0cb256f131), UINT64_C(0x4c05466392e32549), UINT64_C(0x91d3f9324ef03c3e),
    UINT64_C(0xec0591069697e868), UINT64_C(0xa77da4079db8ffd8), UINT64_C(0x287335de3951784f), UINT64_C(0xe7afb90b4adbbf33),
    UINT64_C(0x96e785b0c621dbbf), UINT64_C(0xc7f54753a5e1d81b), UINT64_C(0x4a3a42229fc7491e), UINT64_C(0xc9560ea788a62881),
    UINT64_C(0xe34b9ee97b5bef12), UINT64_C(0xfae309a9fbff0656), UINT64_C(0xbc23f738a0bf4c58), UINT64_C(0xc6dd1ed9a7a706de),
    UINT64_C(0x3473045c7f760007), UINT64_C(0x89b5f0a2e0ace69b), UINT64_C(0x7433c584785f3321), UINT64_C(0xa38220fab7357fc0),
    UINT64_C(0x04e1d70ec8db6456), UINT64_C(0xa86065368c31fd72), UINT64_C(0x926cee3a66885fb3), UINT64_C(0xc09c39dbdb8240bc),
    UINT64_C(0x1ee291407a9ac9db), UINT64_C(0xa6120818b86fd032), UINT64_C(0xa4c3a1cbf6a6666f), UINT64_C(0xb34ce856697db755),
    UINT64_C(0xe3ef1a7123649d75), UINT64_C(0x814ea4e8549f30bc), UINT64_C(0xc8c12f327c1ee0a3), UINT64_C(0xc4ad0d22dbe77043),
    UINT64_C(0x608451fb3ab06a00), UINT64_C(0x2e1141be52867cb9), UINT64_C(0x04b92abd9485965f), UINT64_C(0xcf91f012eb16b951),
    UINT64_C(0xacc0a45db481b3b3), UINT64_C(0x523f65d99013b4d9), UINT64_C(0xf333b8f8613fae1f), UINT64_C(0x8b651a304f1c80b0),
    UINT64_C(0xa91ecd6f061480d2), UINT64_C(0xbd01125685871081), UINT64_C(0x9933950983b6d41e), UINT64_C(0x1f4130fd7912c3e6),
    UINT64_C(0x333230fc9385a4ba), UINT64_C(0x9d2d764680fb1581), UINT64_C(0x277e6bb16761eabf), UINT64_C(0x1829af028f40b602),
    UINT64_C(0x9783144e64561566), UINT64_C(0x410d30cd66cb4e92), UINT64_C(0xce0e0df02a7ac717), UINT64_C(0xdbfc28dabb65c1e2),
    UINT64_C(0x5a83f419f0610b35), UINT64_C(0xb0706efb6f56176b), UINT64_C(0x684148ee29c2a3d6), UINT64_C(0xc47213009755db33),
    UINT64_C(0x2600f460fbea3831), UINT64_C(0x7037ec48a50dc3ec), UINT64_C(0xa761879a39764433), UINT64_C(0xcfd6983de3381424),
    UINT64_C(0xfdc2524f5d605fc4), UINT64_C(0xbe84a33131a412c9), UINT64_C(0x1bd73706e51699b5), UINT64_C(0x7aea62c60dffb5ab),
    UINT64_C(0x010fec687da2bbf4), UINT64_C(0x56aa74a28e54f75c), UINT64_C(0xba52dd2bb4019afe), UINT64_C(0x6ae298d992a98093),
    UINT64_C(0xdbfc6eddb2348c70), UINT64_C(0xeab81b5b034b7836), UINT64_C(0x692b0fc00c8986ba), UINT64_C(0x02adf5476f927b39),
    UINT64_C(0x0173c9bb282a94e7), UINT64_C(0x1e617773e554c877), UINT64_C(0x241d5db92d0aa39e), UINT64_C(0x902c43c4be589249),
    UINT64_C(0x0b817ad8f9617273), UINT64_C(0x43508b7fb53d5d1f), UINT64_C(0xaf1d845886eeb50c), UINT64_C(0xc645d0758b0a08f2),
    UINT64_C(0x3d1339390783be12), UINT64_C(0x376e4919f2fc41c9), UINT64_C(0x392c5bb8475370e6), UINT64_C(0x5e891f54eec6c015),
    UINT64_C(0x16a12880b9ac0923), UINT64_C(0x6437af0453c57f36), UINT64_C(0x8dd1ec0ee82c5835), UINT64_C(0xc4738296f5085ef5),
    UINT64_C(0x68c5d2b2d2d06381), UINT64_C(0x8a4627fb8fbef8df), UINT64_C(0x9d56ea18dd2590b3), UINT64_C(0x8dbdd1fd0ca96586),
    UINT64_C(0x9c17bd827cc151ab), UINT64_C(0xdddb70eb24c36775), UINT64_C(0xb56277dfd02a9c4d), UINT64_C(0x5a8388d255264a83),
    UINT64_C(0xcb7207a0b0155fa4), UINT64_C(0x2bbc2967864dd11a), UINT64_C(0x19fb91190adfc85a), UINT64_C(0xed562d76a7e244c3),
    UINT64_C(0xf5438c5585588610), UINT64_C(0xbc16ff713cde2e48), UINT64_C(0x42248c858cf837cb), UINT64_C(0x59c8eeb9769cf08a),
    UINT64_C(0x0f5260cc1dc624b7), UINT64_C(0x6b880672b5ebfdd5), UINT64_C(0x2e6d6cf57e3365cf), UINT64_C(0xe994b274628cdb20),
    UINT64_C(0x939e00fbb43765d8), UINT64_C(0x093150ef5c7cd883), UINT64_C(0x8ae15f57f13b42f1), UINT64_C(0x3af5014a74f18355),
    UINT64_C(0x7e1a2d0c860bcd23), UINT64_C(0x796312eee1445e38), UINT64_C(0x1cbde8ef8bdfee3d), UINT64_C(0x207592ed0910de04),
    UINT64_C(0x150e839a79142012), UINT64_C(0xb920f5ff40de84a6), UINT64_C(0x0c05b146a932213b), UINT64_C(0x7406c434e2d92546),
    UINT64_C(0x19376004d1fc67aa), UINT64_C(0x82f3677fcf0dd552), UINT64_C(0xd9daf63e3aa745a9), UINT64_C(0x8e1e09d0a9676fdf),
    UINT64_C(0x2cb86571c0289958), UINT64_C(0x4c4c12eb3a97b760), UINT64_C(0x1e3468d9bf56d00c), UINT64_C(0x11f90498f14cb4a4),
    UINT64_C(0x251664b4422a7c58), UINT64_C(0xad10e44d41c2b7c5), UINT64_C(0x663cf17121b6d221), UINT64_C(0x3fe40cdc49c541b8),
    UINT64_C(0xb1b1a8b2a941f9c7), UINT64_C(0x83ffae6e34d4eb78), UINT64_C(0xa4564673c6728fbf), UINT64_C(0xe1499f6bd812a4b9),
    UINT64_C(0xfb5507a915ed36a3), UINT64_C(0xe055a829c62de53c), UINT64_C(0x1ea06fc53acba653), UINT64_C(0xce0f8c15fd8f2258),
    UINT64_C(0x7dd42e43e5ef6f4b), UINT64_C(0x0c55aecd7e1adc10), UINT64_C(0xc31b0e4d3a4e8b1c), UINT64_C(0x1205469d91599780),
    UINT64_C(0xbba5d6df94390b83), UINT64_C(0xc97925cae2f17697), UINT64_C(0x3b98f3dc9e15ea08), UINT64_C(0x878203758954cd36),
    UINT64_C(0x818deaef5ba91f77), UINT64_C(0x6f8f1786214acb89), UINT64_C(0x26c5c2162849ece8), UINT64_C(0xaf1c297b73471dd3),
    UINT64_C(0x415c497c9fa7e936), UINT64_C(0xc1804e923aa3cce6), UINT64_C(0xdd7ca8ffb78dc68c), UINT64_C(0x5b912445ed7ba89a),
    UINT64_C(0x95dec0af89a1f157), UINT64_C(0x7041c032d1fa5266), UINT64_C(0xc569835beabc20df), UINT64_C(0xcc662c0dbb7baaef),
    UINT64_C(0x20d5d2c1383ff75c), UINT64_C(0x7efdaae3e1c4eaaf), UINT64_C(0x3575fad9533be200), UINT64_C(0xfb0fb500836d48dd),
    UINT64_C(0xd211a5090e6d53e2), UINT64_C(0x34afe4050a01467c), UINT64_C(0x63457fe7bfe187c3), UINT64_C(0xc3ee000cb474d925),
    UINT64_C(0x4fd32cbbb8326e22), UINT64_C(0xc2abcd1fc9bf14c2), UINT64_C(0xf34b534e55f28258), UINT64_C(0x094ff2a11972ddec),
    UINT64_C(0x9744b26f181926a9), UINT64_C(0xa7fe6a0982135b29), UINT64_C(0x0f8d9e7a0de7d61b), UINT64_C(0x4bcd12d1b5d3d8a6),
    UINT64_C(0x706e34dbac81bd39), UINT64_C(0xefea01605e9304c6), UINT64_C(0xee3bb6d1e510efe1), UINT64_C(0x84a094db3f4620f8),
    UINT64_C(0xf1752fc679d6aeb3), UINT64_C(0x54921e5d6949a43f), UINT64_C(0xd3616f81f2ff8c55), UINT64_C(0x8bd9584eb62232bd),
    UINT64_C(0xa990035eef6e7b13), UINT64_C(0xd4c56de5c11dcdda), UINT64_C(0x8048c23ec8bd072b), UINT64_C(0x407539904d984e51),
    UINT64_C(0xeaf5a1d46eb3779b), UINT64_C(0x4b06e5769362f357), UINT64_C(0x931f75e21bc0d143), UINT64_C(0x9369439b81c92fc4),
    UINT64_C(0x059fccc0d4afbb45), UINT64_C(0xd072671b3c927118), UINT64_C(0x61b6803f95c41115), UINT64_C(0xacb4b2c4381da3f5),
    UINT64_C(0xd73bf897ee871c72), UINT64_C(0x241c9d52c953d3c0), UINT64_C(0x083c079e704d7b96), UINT64_C(0x8c431ee43e5171a5),
    UINT64_C(0x66079596998b96b6), UINT64_C(0x041ea35d207b478e), UINT64_C(0xbe698683cf7b258e), UINT64_C(0x5457365cf6cbc5bb),
    UINT64_C(0xc166c3ef7006b02d), UINT64_C(0x27789ff1e5365132), UINT64_C(0xae4a02397d308867), UINT64_C(0x0388704d03d7b613),
    UINT64_C(0xf5c9d782d3fd58e3), UINT64_C(0xb51c3fe53965624e), UINT64_C(0xf785b86e7fe0adec), UINT64_C(0x19f72a9ef3a215e8),
    UINT64_C(0x19db58361e6633d9), UINT64_C(0xf1fe7a08693d64ab), UINT64_C(0x07c3310adc3bbf03), UINT64_C(0x742e87d333077816),
    UINT64_C(0xe817529af0f04970), UINT64_C(0xe7f343c941a044ff), UINT64_C(0xf9693fb4f37b4d2c), UINT64_C(0xb99da4a0b6ccb1ed),
    UINT64_C(0x4eef654d39c7f631), UINT64_C(0xd06badd9354befc8), UINT64_C(0x3dea38b48a4fb6cf), UINT64_C(0xf6551a2de11ec63d),
    UINT64_C(0xf0dd7ca2d08731e5), UINT64_C(0xfbbac6e989684aff), UINT64_C(0xe2b65b698f6ea652), UINT64_C(0x679e2fc32595fb51),
    UINT64_C(0x6547fdc240571414), UINT64_C(0x6809f663de2d0466), UINT64_C(0x6c6b7a0a40a5e48f), UINT64_C(0xe5f43660d891606e),
    UINT64_C(0xa44f283a5a5c10fd), UINT64_C(0x95635b53a60083be), UINT64_C(0x7e0f003a2698a45c), UINT64_C(0x2fd0eb2a3cb4db79),
    UINT64_C(0x7416380640ad33c7), UINT64_C(0x988de04a8bfe794b), UINT64_C(0x6d00569ebd6839ff), UINT64_C(0x22ddd7d3d0efa384),
    UINT64_C(0x20f9c1ae73b1a651), UINT64_C(0x32386da97bb626af), UINT64_C(0x263c358b8e1975fe), UINT64_C(0x32bd1e4fdb3e7f7c),
    UINT64_C(0x2ebb53af95ab07db), UINT64_C(0xeccc526f7e6aca61), UINT64_C(0x186fd1f3ad161e28), UINT64_C(0xf96dd58eca026372),
    UINT64_C(0x0403c8572fee3bf3), UINT64_C(0x2598261d29b22e84), UINT64_C(0xa4027ffeed481ae0), UINT64_C(0xe2f690ddcdb0fdaf),
    UINT64_C(0x95d11d0d60c528fd), UINT64_C(0x0cc242f0eeae1d6c), UINT64_C(0xfa3440087835377f), UINT64_C(0x3d8fad475b8139e4),
    UINT64_C(0x8e92fce862d8a97e), UINT64_C(0xc53bc4cb5ed50eb4), UINT64_C(0xc8f91ece0194e8d4), UINT64_C(0xf78d7c6b5cff07e1),
    UINT64_C(0x3163d8458b924665), UINT64_C(0xc2ae6dc185c739bf), UINT64_C(0x2943e3eae337c6c6), UINT64_C(0x96bd36f0da4a49f7),
    UINT64_C(0x98753f33282f27bf), UINT64_C(0xd5c33455bf0f69fd), UINT64_C(0x78cc9f69e0286682), UINT64_C(0x0631fadc21ec437c),
    UINT64_C(0x521c3db58b6b1170), UINT64_C(0x2333f0f70e46b5cf), UINT64_C(0x87be027b8d434ac6), UINT64_C(0xba4c26796c582e4c),
    UINT64_C(0x35d52e4f85db73e4), UINT64_C(0x8ac3723b24e99436), UINT64_C(0x4a2b6ce4b7a97a02), UINT64_C(0xcb8017cc584b287d),
    UINT64_C(0x1ca3610bc2f30e9f), UINT64_C(0xc1c2dafdd385b283), UINT64_C(0xa812778eceff9a2b), UINT64_C(0x91b8429959ef5359),
    UINT64_C(0xa2750c665bcab7d2), UINT64_C(0x9212f5d704b5320b), UINT64_C(0xfa46bb7a213be57f), UINT64_C(0xd20cbd122dce6c1d),
    UINT64_C(0x82868b5aee7a4776), UINT64_C(0xf49ec5ddf8cec096), UINT64_C(0xa4fc2bf71ac9dcc2), UINT64_C(0x9d8b8f462bd2f17b),
    UINT64_C(0x452703fe91008332), UINT64_C(0x919a288ada854bef), UINT64_C(0x75d2b2eb0f4eeed7), UINT64_C(0xd64885293558a96f),
    UINT64_C(0x098d7efb4f8d5b31), UINT64_C(0x7ee77eef93a3928e), UINT64_C(0xb28eebae28b63dc8), UINT64_C(0x0f01129fc90af970),
    UINT64_C(0xf3d5b92900d45181), UINT64_C(0xb9d8a408ea6715c0), UINT64_C(0xe44424fb8ca9e22e), UINT64_C(0xd81135834c1aaf96),
    UINT64_C(0x445b3d67398e888b), UINT64_C(0x0dad43784fe36cda), UINT64_C(0xe6d1bd75c5d81518), UINT64_C(0x662f0e924150c5cb),
    UINT64_C(0x78179f80df6e0709), UINT64_C(0xdd8fc687a741289c), UINT64_C(0x710873d7f5ab060e), UINT64_C(0xa1961d2b538f497c),
    UINT64_C(0xb36bbf75bc8b8761), UINT64_C(0x675c608353017307), UINT64_C(0xade6b1aa0ec59bbe), UINT64_C(0xc803a2c9426b3c5f),
    UINT64_C(0x48a8210409b5ffac), UINT64_C(0xc3d58389ce5f3b13), UINT64_C(0xa23ceb0e71b08443), UINT64_C(0xd9d192cd9c5e9a05),
    UINT64_C(0x20d9cd878b94147d), UINT64_C(0x22329c7695f6df46), UINT64_C(0xaebdcdc2c2cbc0d9), UINT64_C(0xe95ae3d514f6f94b),
    UINT64_C(0x59152e1f5715e782), UINT64_C(0xb3280d75a8134f15), UINT64_C(0x5bce3379e1fcb7b4), UINT64_C(0x437d9c3238c4169f),
    UINT64_C(0x77db7e5ebd5125bd), UINT64_C(0x0dd3aef40336d438), UINT64_C(0x4a496a56bac81428), UINT64_C(0x72a128c3875dc93d),
    UINT64_C(0x8eb605e5bef1747d), UINT64_C(0x666d4546567a4eef), UINT64_C(0xad5ad003399d2296), UINT64_C(0x19c74366682b52a0),
    UINT64_C(0xb3c35c5a0e259420), UINT64_C(0xf98340503eb93d6d), UINT64_C(0xa51985b0bb7f81e8), UINT64_C(0x2a21510c6c7ca42f),
    UINT64_C(0x3c1ac0b52c230998), UINT64_C(0x4e1d572a2d77000b), UINT64_C(0x8dd3adff3bfdec71), UINT64_C(0xdfb3a4a23e43d035),
    UINT64_C(0xe12f748421173e62), UINT64_C(0x2f356145d2f72758), UINT64_C(0x31c13682374c445c), UINT64_C(0x09240a1f409fab88),
    UINT64_C(0xa346e2d2f72fd5e8), UINT64_C(0x2c5b53bfc05f9f77), UINT64_C(0x0a9f7ab218574f6e), UINT64_C(0xc3fcb9b977f0cceb),
    UINT64_C(0xac26889eb86459b9), UINT64_C(0x1082f785bc3dac21), UINT64_C(0x3c8c337a4c67ef18), UINT64_C(0x118e48d0e8a66e02),
    UINT64_C(0xb777cef85278f2dc), UINT64_C(0x12a268a3dcda05bc), UINT64_C(0x75f5f7d3fde0bd9e), UINT64_C(0x62f5f1650ec91670),
    UINT64_C(0x81fcf9e3e1c3adec), UINT64_C(0xf0b5e35ace23349c), UINT64_C(0xde7d514d058e53a4), UINT64_C(0x52a625e5f06242c7),
    UINT64_C(0x3cc1346eda6a430a), UINT64_C(0x165bd737e851f6a1), UINT64_C(0xe52c53d745f1b49a), UINT64_C(0x15513074f676fafc),
    UINT64_C(0xcb8797dbb29e6710), UINT64_C(0x27b92c8190fd679d), UINT64_C(0x0b39384ac668b176), UINT64_C(0x11341e6d7adad0e9),
    UINT64_C(0x491b5b5390b70f94), UINT64_C(0x1f5eccf586d03746), UINT64_C(0x6502ca945646feae), UINT64_C(0x3abb5466229ef7d8),
    UINT64_C(0x535b4effbe0ce5f6), UINT64_C(0x6575eefef9e916f5), UINT64_C(0x77a76fbf3c76f2d7), UINT64_C(0x1cc63124152994a7),
    UINT64_C(0x6e33f80e95d4323d), UINT64_C(0xd711791d9b2e1d65), UINT64_C(0x7c766cd52013ae49), UINT64_C(0x08bc15230d2ef477),
    UINT64_C(0xb751fa3b942ab063), UINT64_C(0xfe99a8b170a11941), UINT64_C(0x731979294908218a), UINT64_C(0x32166899c12f3097),
    UINT64_C(0x8318df8e3823dd3d), UINT64_C(0x940e81f0b4ece3d8), UINT64_C(0x81ea0f12130235ea), UINT64_C(0x36603dfef356d752),
    UINT64_C(0x409eeb16b992d793), UINT64_C(0xf4c675cca09e229a), UINT64_C(0x0ef989d732dae818), UINT64_C(0x269b4385573ad2f6),
    UINT64_C(0x53df04584157173c), UINT64_C(0x260c347bedc5ce82), UINT64_C(0xb9fbfba9b58c1b09), UINT64_C(0x20115df9d0693a14),
    UINT64_C(0x8c0fb27588303369), UINT64_C(0x3a9450974a66eaaf), UINT64_C(0x805f0d515d715679), UINT64_C(0x10f4b52a09898972),
    UINT64_C(0x20e9c3449e84718e), UINT64_C(0x9eed8745b4e234e2), UINT64_C(0x946c3083bf840def), UINT64_C(0xb18de02e626f7dd9),
    UINT64_C(0x9e8b496b1d035ed8), UINT64_C(0x6ef3891e7c690f77), UINT64_C(0xd62269e5ad1c07f5), UINT64_C(0x7117ed7eddc2883e),
    UINT64_C(0x260f1d08457dfcca), UINT64_C(0xe0759189d723da9d), UINT64_C(0xd6d40adb9c9f94d7), UINT64_C(0x7c47c4b4a670b77e),
    UINT64_C(0xb2b5179563a2abe1), UINT64_C(0x62118cb60f121507), UINT64_C(0x22c3a4a74379ceb1), UINT64_C(0xd5904c844fbfed74),
    UINT64_C(0xa0afa38c06d50d92), UINT64_C(0xd6223dbbcfcf73f4), UINT64_C(0xf19623e7ec6f83dd), UINT64_C(0xd08c12de2b6265f6),
    UINT64_C(0xc487d5dc19489db6), UINT64_C(0x759283ffd06fc796), UINT64_C(0xd61a735ad1cd7ccc), UINT64_C(0x32084ba3ca8fa3ee),
    UINT64_C(0x17530308a1204968), UINT64_C(0x80328582a1eb8d8f), UINT64_C(0xd4c873deec7fb3d7), UINT64_C(0x11c825cc4bc8b181),
    UINT64_C(0x0137fa50576b21eb), UINT64_C(0xc5ea2f958a3ddb53), UINT64_C(0x6ae611d92b67c9bc), UINT64_C(0xb798b3e1f9c3a851),
    UINT64_C(0x22a42679fa4b013f), UINT64_C(0x2071f22dae8de629), UINT64_C(0x3faa3a80e45cbca6), UINT64_C(0xb0418f45808009ec),
    UINT64_C(0x446063013dd5a0f4), UINT64_C(0x932445b680ef71ec), UINT64_C(0x2bc9a2d9ab8e2662), UINT64_C(0x8ebd57fbc56a6154),
    UINT64_C(0xa28f3d2264ad0f10), UINT64_C(0xffff84df76a10c15), UINT64_C(0xac5c9b0e78fbee81), UINT64_C(0xc1f08e08982b237c),
    UINT64_C(0x5907b7fa41daa2b8), UINT64_C(0xbed3856320d9c3c2), UINT64_C(0x500a342c1902f015), UINT64_C(0x0c3a5d539c71b7d6),
    UINT64_C(0xa706750b1c3e5604), UINT64_C(0x1543ab593a8c824c), UINT64_C(0xbdfd9d26f151d83c), UINT64_C(0x1603bb40537de208),
    UINT64_C(0x1501b0ba802daa2d), UINT64_C(0xdcbcc803f3c11f3c), UINT64_C(0x2bb283a389ec2f35), UINT64_C(0x3a27513ef9d14bf4),
    UINT64_C(0xcb7c4fd02a39d8af), UINT64_C(0xcc6f61a03488e43f), UINT64_C(0xfdddf2b5fd6c4b05), UINT64_C(0xa015987625b9755d),
    UINT64_C(0x14c5a9b03c63b253), UINT64_C(0x413f7d2608bf939e), UINT64_C(0x8bdb68c7176407e5), UINT64_C(0x436de64d8a614c32),
    UINT64_C(0xc2aca4b10ff0bf8e), UINT64_C(0x3b56cc9c1df797e4), UINT64_C(0xb1750cce6cca57bb), UINT64_C(0x8c80e2303509012a),
    UINT64_C(0x7f25bae3c4fea8af), UINT64_C(0xecf8ed9dac1367b8), UINT64_C(0x1a49274e39668f4e), UINT64_C(0xca4a0ae881c7dc39)
};

static const int      STATE = 32;
static const uint64_t MASK  = UINT64_C(0xffffffffffffff);

//--------
// State mix function
static FORCE_INLINE uint8_t beam_ROTR8( uint8_t v, int n ) {
    n = n & 7;
    if (n) {
        v = (v >> n) | (v << (8 - n));
    }
    return v;
}

static FORCE_INLINE uint64_t beam_ROTR64( uint64_t v, int n ) {
    n = n & 63;
    if (n) {
        v = ROTR64(v, n);
    }
    return v;
}

static FORCE_INLINE void mix( uint64_t * state, const uint32_t A ) {
    const uint32_t B  = A + 1;
    const uint32_t iv = state[A] & 1023;
    const uint64_t M  = T[iv];

    state[B] += state[A] + M;

    state[A] ^= state[B];
    state[B] ^= state[A];
    state[A] ^= state[B];

    state[B]  = beam_ROTR64(state[B], state[A]);
}

//---------
// Hash round function
template <bool bswap>
static FORCE_INLINE void round( uint64_t * const state, const uint8_t * m8, uint32_t len ) {
    uint8_t * const state8 = (uint8_t *)state;
    uint32_t        index  = 0;
    uint32_t        sindex = 0;

    for (uint32_t Len = len >> 3; index < Len; index++) {
        uint64_t blk = GET_U64<bswap>(m8, index * 8);
        state[sindex] += beam_ROTR64(blk + index + 1, state[sindex] + index + 1);
        if (sindex == 1) {
            mix(state, 0);
        } else if (sindex == 3) {
            mix(state, 2);
            sindex = -1;
        }
        sindex++;
    }

    mix(state, 0);

    index <<= 3;
    sindex  = index & 31;
    for (; index < len; index++) {
        const uint32_t ssindex = bswap ? (sindex ^ 7) : sindex;
        state8[ssindex] += beam_ROTR8(m8[index] + index + 1, state8[ssindex] + index + 1);
        // state+[0,1,2]
        mix(state, index % 3);
        if (sindex >= 31) {
            sindex = -1;
        }
        sindex++;
    }

    mix(state, 0);
    mix(state, 1);
    mix(state, 2);
}

//---------
// main hash function
template <bool bswap>
static void beamsplitter_64( const void * in, const size_t len, const seed_t seed, void * out ) {
    const uint8_t * key8Arr    = (uint8_t *)in;
    uint32_t        seedbuf[2] = { 0 };

    if (len >= UINT32_C(0xffffffff)) { return; }

    // the cali number from the Matrix (1999)
    uint32_t seed32 = seed;
    if (!bswap) {
        seedbuf[0]  = 0xc5550690;
        seedbuf[0] -= seed32;
        seedbuf[1]  = ~(1 - seed32);
    } else {
        seedbuf[1]  = 0xc5550690;
        seedbuf[1] -= seed32;
        seedbuf[0]  = ~(1 - seed32);
    }

    uint64_t state[STATE / 8];
    // nothing up my sleeve
    state[0] = UINT64_C(0x123456789abcdef0);
    state[1] = UINT64_C(0x0fedcba987654321);
    state[2] = UINT64_C(0xaccadacca80081e5);
    state[3] = UINT64_C(0xf00baaf00f00baaa);

    // The mixing in of the seed array does not need bswap set, since
    // the if() above will order the bytes correctly for that variable.
    round<bswap>(state, key8Arr, (uint32_t)len);
    round<bswap>(state, key8Arr, (uint32_t)len);
    round<bswap>(state, key8Arr, (uint32_t)len);
    round<false>(state, (uint8_t *)seedbuf, 8 );
    round<false>(state, (uint8_t *)seedbuf, 8 );
    round<bswap>(state, key8Arr, (uint32_t)len);
    round<bswap>(state, key8Arr, (uint32_t)len);
    round<bswap>(state, key8Arr, (uint32_t)len);

    /*
     * //printf("state = %#018" PRIx64 " %#018" PRIx64 " %#018" PRIx64 " %#018" PRIx64 "\n",
     * //  state[0], state[1], state[2], state[3] );
     */

    // printf("state = %#018" PRIx64 " %#018" PRIx64 "\n",
    //  state[0], state[1] );

    uint64_t h[2] = { 0 };

    // The new combination step
    h[0]  = state[2];
    h[1]  = state[3];

    h[0] += h[1];

    PUT_U64<bswap>(h[0], (uint8_t *)out, 0);
}

REGISTER_FAMILY(beamsplitter,
   $.src_url    = "https://github.com/crisdosyago/beamsplitter",
   $.src_status = HashFamilyInfo::SRC_STABLEISH
 );

// Yes, this has no bad seeds! See note at the top near "thread_local".
REGISTER_HASH(beamsplitter,
   $.desc       = "A possibly universal hash made with a 10x64 s-box",
   $.hash_flags =
         FLAG_HASH_LOOKUP_TABLE     |
         FLAG_HASH_SMALL_SEED,
   $.impl_flags =
         FLAG_IMPL_VERY_SLOW        |
         FLAG_IMPL_ROTATE_VARIABLE  |
         FLAG_IMPL_LICENSE_MIT,
   $.bits = 64,
   $.verification_LE = 0x1BDF358B,
   $.verification_BE = 0x4791907E,
   $.hashfn_native   = beamsplitter_64<false>,
   $.hashfn_bswap    = beamsplitter_64<true>,
   $.badseeds        = {}
 );
