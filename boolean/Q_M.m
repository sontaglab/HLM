function [ rslt ] = Q_M( N, m, d )
%lgcExpr    return the simplist expression of a logic function
%   N: number of elements (<=10)
%   m: list of minterms 
%   d: list of don't care terms 

if max(max(m), max(d)) >= 2^N,
    disp('Error: N is too small!');
    return;
end

alphabet = 'abcdefghijklmnopqrst';
m = unique(sort(m));
d = unique(sort(d));
mLen = length(m);
dLen = length(d);

if mLen + dLen == 2^N,
	lgcExpr = '1';
	return;
end

% all k * N matrix except cmb_flg
binstr = [];
cmb_flg = zeros(mLen + dLen, 1);
nextbinstr = [];
final = [];

% initializing
bisntr1 = dec2bin(m, N);
binstr2 = dec2bin(d, N);
binstr = [bisntr1; binstr2];


% combining 
while 1,
    countnew = 0;
    Len = size(binstr, 1);
    for p = 1:(Len - 1),
        for q = (p + 1):Len,
            notEqual = (binstr(p,:) ~= binstr(q,:));
            if sum(notEqual) == 1,
                countnew = countnew + 1;
                cmb_flg(p) = 1;
                cmb_flg(q) = 1;
                tmp = binstr(p,:);
                tmp(notEqual) = '-';
                nextbinstr = [nextbinstr; tmp]; % may get repeated binstr
            end
        end
    end
    for k = 1:Len,
        if cmb_flg(k) == 0,
            final = [final; binstr(k,:)];
        end
    end
    if countnew == 0,
        break;
    end
    cmb_flg = zeros(countnew, 1);
    binstr = nextbinstr;
    binstr = unique(binstr, 'rows');    % eliminate repeated rows in time
    nextbinstr = [];
end
final = unique(final, 'rows');

% Petrick's Method
rslt = [];
rw = size(final, 1);
cl = mLen;
ptk = zeros(rw, cl);
for p = 1:cl,
    for q = 1:rw, 
        vec1 = dec2bin(m(p), N);
        vec2 = final(q, :);
        neq = (vec1 ~= vec2);
        tmp = unique(vec2(neq));
        if ((length(tmp) == 1) && (tmp(1) == '-')) || (isempty(tmp)),
            ptk(q, p) = 1;
        end
    end
end
k = 1;
while k <= size(ptk, 2),
    id = find(ptk(:, k));
    if length(id) > 1,
        k = k + 1;
        continue;
    end
    % length(id) == 1
    idr = find(ptk(id, :));
    ptk(:, idr) = [];
    ptk(id,:) = [];
    rslt = [rslt; final(id,:)];
    final(id, :) = [];
end

if size(ptk, 2) > 0,
% ptk is not empty yet
rw = size(ptk, 1);
cbcell = {};
rws = 1:rw;
for k = 1:size(ptk, 2),
	tmp = (ptk(:, k) == ones(rw, 1));
	cbcell = [cbcell rws(tmp)];
end
sz = zeros(length(cbcell), 1);
for k = 1:length(cbcell),
	sz(k) = size(cbcell{k}, 2);
end
cl = length(cbcell);
lst = zeros(prod(sz), cl);
cbarr = ones(1,cl);%ones(1, size(lst, 1))
%cbcell
for k = 1:size(lst, 1),
	tmp = zeros(1, cl);
	for kk = 1:cl,
		tmp(kk) = cbcell{kk}(cbarr(kk));
	end
	tmp = sort(tmp);
	lst(k, :) = tmp;
	cbarr(1) = cbarr(1) + 1;
	q = 1;
	while cbarr(q) > sz(q),
        % elements of cbcell have varied sizes
        % have no choice but to simulate rounds with while loops
		cbarr(q) = 1;
		q = q + 1;
		if q > cl,
			break;
		end
		cbarr(q) = cbarr(q) + 1;
	end
end 

lst = unique(lst, 'rows');
rw = size(lst, 1);
Len1 = size(unique(lst(1, :)), 2);
Len2 = sum(final(unique(lst(1, :))) ~= '-');
r = 1;
for k = 1:rw,
	tlen1 = size(unique(lst(k, :)), 2);
	if tlen1 < Len1, 
		Len1 = tlen1;
		Len2 = sum(final(unique(lst(k, :)), :) ~= '-');
		r = k;
	else
		if tlen1 == Len1, 
			tlen2 = sum(final(unique(lst(k, :)), :) ~= '-');
			if tlen2 < Len,
				Len2 = tlen2;
				Len1 = size(unique(lst(k, :)), 2);
				r = k;
			end
		end 
	end 
end 
vec = unique(lst(r,:));
for k = 1:length(vec),
    rslt = [rslt; final(vec(k),:)];
end

end     % finish dealing with unempty ptk

% output
% lgcExpr = [];
% for k = 1:size(rslt, 1),
%     vec = rslt(k,:);
%     for p = 1:N,
%         if vec(p) == '-',
%             continue;
%         end
%         if vec(p) == '1',
%             lgcExpr = [lgcExpr alphabet(p)];
%         else if vec(p) == '0',
%             lgcExpr = [lgcExpr alphabet(p) ''''];
%             end
%         end
%     end
%     if k ~= size(rslt, 1),
%         lgcExpr = [lgcExpr '+'];
%     end
% end

end